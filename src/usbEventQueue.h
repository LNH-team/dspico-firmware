#pragma once

enum
{
    USB_EVENT_NONE = 0,
    USB_EVENT_BUS_RESET = 1,
    USB_EVENT_UNPLUGGED = 2,
    USB_EVENT_SOF = 0x20000000,
    USB_EVENT_SUSPEND = 3,
    USB_EVENT_RESUME = 4,
    USB_EVENT_SETUP_RECEIVED = 0x40000000,
    USB_EVENT_XFER_COMPLETE = 0x30000000
};

#define USB_EVENT_QUEUE_LENGTH      64

/// @brief Struct representing a USB event queue.
typedef struct
{
    volatile u8 readPtr;
    volatile u8 writePtr;
    u32 queue[USB_EVENT_QUEUE_LENGTH];
} usb_event_queue_t;

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Clears the USB event queue.
void usb_clearEventQueue(void);

/// @brief Prepares for dequeueing a USB event.
void usb_prepareDequeueEvent(void);

/// @brief Dequeues a USB event and returns it.
/// @return The dequeued USB event, or \see USB_EVENT_NONE when empty.
u32 usb_dequeueEvent(void);

/// @brief Tries to enqueue a 32-bit USB \p event.
/// @param event The event to enqueue.
/// @return \c true when enqueueing was successful, or \c false otherwise.
bool usb_tryEnqueueEvent32Bit(u32 event);

/// @brief Tries to enqueue a 64-bit USB \p event.
/// @param event0 The first part of the event to enqueue.
/// @param event1 The second part of the event to enqueue.
/// @return \c true when enqueueing was successful, or \c false otherwise.
bool usb_tryEnqueueEvent64Bit(u32 event0, u32 event1);

#ifdef __cplusplus
}
#endif
