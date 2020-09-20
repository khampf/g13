#ifndef __G13_H__
#define __G13_H__


#include "g13_device.hpp"
#include "g13_action.hpp"
#include "g13_log.hpp"
#include "g13_manager.hpp"
#include <libusb-1.0/libusb.h>

// *************************************************************************

namespace G13 {

    static const size_t G13_VENDOR_ID = 0x046d;
    static const size_t G13_PRODUCT_ID = 0xc21c;
    static const size_t G13_REPORT_SIZE = 8;

    // const size_t G13_INTERFACE = 0;
    static const size_t G13_KEY_ENDPOINT = 1;
    static const size_t G13_LCD_ENDPOINT = 2;
    static const size_t G13_KEY_READ_TIMEOUT = 100;
    static const size_t G13_KEY_READ_MIN_TIME = 0;

    // *************************************************************************

    class G13_CommandException : public std::exception {
    public:
        explicit G13_CommandException(std::string reason) : _reason(std::move(reason)) {}

        ~G13_CommandException() noexcept override = default;

        [[nodiscard]] const char *what() const noexcept override { return _reason.c_str(); }

        std::string _reason;
    };

}  // namespace G13

#endif  // __G13_H__
