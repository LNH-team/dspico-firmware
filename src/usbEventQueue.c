#include "common.h"
#include "hardware/gpio.h"
#include "usbEventQueue.h"

static usb_event_queue_t sUsbEventQueue;

static void setCartridgeIrqState(bool asserted)
{
    gpio_put(PIN_IRQ, asserted);
}

void usb_clearEventQueue(void)
{
    sUsbEventQueue.readPtr = 0;
    sUsbEventQueue.writePtr = 0;
    setCartridgeIrqState(false);
}

void usb_prepareDequeueEvent(void)
{
    u32 readPtr = sUsbEventQueue.readPtr;
    u32 writePtr = sUsbEventQueue.writePtr;
    u32 nextReadPtr = (readPtr + 1) % USB_EVENT_QUEUE_LENGTH;
    if (readPtr == writePtr || nextReadPtr == writePtr)
    {
        setCartridgeIrqState(false);
    }
}

u32 usb_dequeueEvent(void)
{
    u32 readPtr = sUsbEventQueue.readPtr;
    u32 writePtr = sUsbEventQueue.writePtr;
    if (readPtr == writePtr)
    {
        return USB_EVENT_NONE;
    }
    else
    {
        u32 result = sUsbEventQueue.queue[readPtr];
        u32 nextReadPtr = (readPtr + 1) % USB_EVENT_QUEUE_LENGTH;
        sUsbEventQueue.readPtr = nextReadPtr;
        if (nextReadPtr == writePtr)
        {
            setCartridgeIrqState(false);
            result |= 1u << 31;
        }
        return result;
    }
}

bool usb_tryEnqueueEvent32Bit(u32 event)
{
    u32 readPtr = sUsbEventQueue.readPtr;
    u32 writePtr = sUsbEventQueue.writePtr;
    u32 nextWritePtr = (writePtr + 1) % USB_EVENT_QUEUE_LENGTH;
    if (readPtr == nextWritePtr)
    {
        return false;
    }
    else
    {
        sUsbEventQueue.queue[writePtr] = event;
        sUsbEventQueue.writePtr = nextWritePtr;
        setCartridgeIrqState(true);
        return true;
    }
}

bool usb_tryEnqueueEvent64Bit(u32 event0, u32 event1)
{
    u32 readPtr = sUsbEventQueue.readPtr;
    u32 writePtr = sUsbEventQueue.writePtr;
    u32 nextWritePtr = (writePtr + 1) % USB_EVENT_QUEUE_LENGTH;
    u32 nextNextWritePtr = (nextWritePtr + 1) % USB_EVENT_QUEUE_LENGTH;
    if (readPtr == nextWritePtr || readPtr == nextNextWritePtr)
    {
        return false;
    }
    else
    {
        sUsbEventQueue.queue[writePtr] = event0;
        sUsbEventQueue.queue[nextWritePtr] = event1;
        sUsbEventQueue.writePtr = nextNextWritePtr;
        setCartridgeIrqState(true);
        return true;
    }
}
