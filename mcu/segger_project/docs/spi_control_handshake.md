# SPI capture vs. control-task scheduling

The MCU has to pull in 9.6 kB of masked pixels every 33 ms, but the FPGA can
push a complete frame in roughly 2 ms. The safest approach is to make the SPI
interface entirely interrupt-driven so that the DMA channel moves bytes from the
SPI data register into RAM while the CPU is free. When the DMA controller asserts
its half-transfer or transfer-complete interrupts, the CPU is notified via the
`DMA2_Channel3_IRQn` ISR, which calls the HAL DMA handler and, in turn, the SPI
RX callbacks defined in `spi.c`:

- `HAL_SPI_RxHalfCpltCallback()` sets `spi_rx_half_complete`
- `HAL_SPI_RxCpltCallback()` sets `spi_rx_full_complete`
- `HAL_SPI_ErrorCallback()` sets `spi_rx_error`

Because those callbacks execute from the DMA interrupt context they always run
before any lower-priority work. In `SPI1_DMA_Init()` the NVIC priority of the DMA
interrupt is set to `(0,0)`, which is the highest urgency in the system. That
means the "SPI read task" (really the DMA ISR that services SPI1) preempts the
foreground code and therefore effectively has a higher priority than the control
algorithm.

The foreground ("control") code simply polls the flags that the DMA ISR raises.
As soon as `spi_rx_full_complete` becomes `true`, the SPI DMA stream has finished
copying the frame into `frame_buffer[]`, so the control algorithm can safely run
on that buffer for the remaining ~31 ms. The control code should do three things
in the order below:

1. Clear the flag(s) raised by the ISR so the next frame can reuse them.
2. Invoke its control loop / frame-processing routine while DMA is idle.
3. Kick off the next DMA receive by calling `SPI1_Receive_DMA()` once processing
   is done.

That pattern is already present in `main.c`:

```c
if (spi_rx_full_complete) {
    spi_rx_full_complete = false;
    ProcessFrame(frame_buffer, BUFFER_SIZE); // control logic
    SPI1_Receive_DMA(frame_buffer, BUFFER_SIZE); // restart capture
}
```

You can expand `ProcessFrame()` into your full control task—just keep in mind
that it executes in the main loop, so it should not block the DMA interrupt. If
you later migrate to an RTOS you can replace the polling with a direct-from-ISR
notification (binary semaphore, task notification, etc.), but the concept stays
the same: the SPI DMA interrupt runs at the highest priority to capture data,
and it signals the lower-priority control task only after the frame buffer is
ready.

The firmware now ships with `spi_control_handshake.c/.h`, a tiny helper that
codifies the pattern above. Call `SpiControlHandshake_BeginCapture()` to kick
off the SPI DMA transfer, `SpiControlHandshake_Service()` from the foreground
loop to consume the ISR flags, and `SpiControlHandshake_FrameReady()` to know
when your control task can safely run. After processing, release the buffer via
`SpiControlHandshake_ReleaseFrame()` and begin the next capture. This mirrors
the flow chart above while keeping the SPI/DMA interrupt at the highest NVIC
priority so it always preempts the control code when a frame is arriving.

In short: yes, you *should* use SPI/DMA interrupts. Let the interrupt handler
own the SPI task and use the completion flag(s) or an RTOS notification to wake
up the control code after each frame so you can spend the remaining frame period
on your control algorithm.
