#include <cstddef>
#include <cstring>
#include <string>
#include <xcb/xproto.h>
extern "C" {
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
}
#include <iostream>

int main() {
    std::cout << "Connecting" << std::endl;
    int screen_n = 1;
    xcb_connection_t *c = xcb_connect(nullptr, &screen_n);
    xcb_screen_t *s = xcb_setup_roots_iterator(xcb_get_setup(c)).data;

    xcb_ewmh_connection_t ewmh;
    xcb_intern_atom_cookie_t *cookie = xcb_ewmh_init_atoms(c, &ewmh);
    if (!xcb_ewmh_init_atoms_replies(&ewmh, cookie, nullptr)) {
        std::cerr << "Failed to initialize EWMH atoms" << std::endl;
        return 1;
    }


    auto names = xcb_ewmh_get_desktop_names(&ewmh, screen_n);
    xcb_ewmh_get_utf8_strings_reply_t names_reply;
    if (xcb_ewmh_get_utf8_strings_reply(&ewmh, names, &names_reply, nullptr)) {
        std::cout << "Desktop names: ";
        for (unsigned int i = 0; i < names_reply.strings_len; ++i) {
            std::cout << names_reply.strings[i] << '\0';
        }
        std::cout << std::endl;
        xcb_ewmh_get_utf8_strings_reply_wipe(&names_reply);
    } else {
        std::cerr << "Failed to get desktop names." << std::endl;
    }

    auto num_of_ds = xcb_ewmh_get_number_of_desktops(&ewmh, screen_n);
    uint32_t num_of_desktops;
    if (xcb_ewmh_get_number_of_desktops_reply(&ewmh, num_of_ds, &num_of_desktops, nullptr)){
        std::cout << "Number of desktops: " <<  num_of_desktops <<std::endl;
    }


    auto cur_ck = xcb_ewmh_get_current_desktop(&ewmh, screen_n);
    uint32_t cur = 0;
    if (xcb_ewmh_get_number_of_desktops_reply(&ewmh, cur_ck, &cur, nullptr)){
        std::cout << "Current desktop: " << cur + 1 <<std::endl;
    }

    auto wa_cookie = xcb_ewmh_get_workarea(&ewmh, screen_n);
    xcb_ewmh_get_workarea_reply_t workarea_reply;
    if (xcb_ewmh_get_workarea_reply(&ewmh, wa_cookie, &workarea_reply, nullptr)) {
        std::cout << "_NET_WORKAREA values: ";
        for (unsigned int i = 0; i < workarea_reply.workarea_len; ++i) {
            std::cout << "[" << workarea_reply.workarea[i].x << ", "
                      << workarea_reply.workarea[i].y << ", "
                      << workarea_reply.workarea[i].width << ", "
                      << workarea_reply.workarea[i].height << "] ";
        }
        std::cout << std::endl;
        xcb_ewmh_get_workarea_reply_wipe(&workarea_reply);
    } else {
        std::cerr << "Failed to get _NET_WORKAREA." << std::endl;
    }
}

