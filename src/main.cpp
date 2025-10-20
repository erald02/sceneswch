#include "fmt/base.h"
#include <cstddef>
#include <cstring>
#include <iterator>
#include <string>
#include <xcb/xproto.h>
extern "C" {
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
}
#include <iostream>
#include <toml++/toml.hpp>

const int SCREEN_N = 0;
constexpr uint32_t ALL_DESKTOPS = 0xFFFFFFFF;

int get_number_of_desktops(xcb_ewmh_connection_t *ewmh){
    auto num_of_ds = xcb_ewmh_get_number_of_desktops(ewmh, SCREEN_N);
    uint32_t num_of_desktops;
    if (xcb_ewmh_get_number_of_desktops_reply(ewmh, num_of_ds, &num_of_desktops, nullptr)){
        return num_of_desktops;
    }
    return -1;
}

int get_current_desktop(xcb_ewmh_connection_t *ewmh){
    auto cur_ck = xcb_ewmh_get_current_desktop(ewmh, SCREEN_N);
    uint32_t cur = 0;
    if (xcb_ewmh_get_number_of_desktops_reply(ewmh, cur_ck, &cur, nullptr)){
        return cur;
    }
    return -1;
}

// std::string p_from_pid(int pid){

// }

#define GEOM_TO_TOML(g) toml::table{ \
    {"x", g->x}, {"y", g->y}, {"w", g->width}, {"h", g->height}, \
    {"depth", g->depth}, {"border_width", g->border_width} \
}

bool capture_desktop(xcb_connection_t *c, xcb_ewmh_connection_t *ewmh) {
    int curr_desktop = get_current_desktop(ewmh);
    toml::table config;

    auto nc_cookie = xcb_ewmh_get_client_list(ewmh, SCREEN_N);
    xcb_ewmh_get_windows_reply_t client_list;
    if (xcb_ewmh_get_client_list_reply(ewmh, nc_cookie, &client_list, nullptr)){
        for (unsigned int i = 0; i < client_list.windows_len; ++i){
            // check window is in active desktop
            auto ck = xcb_ewmh_get_wm_desktop(ewmh, client_list.windows[i]);
            uint32_t d;
            if (!xcb_ewmh_get_wm_desktop_reply(ewmh, ck, &d, nullptr)) continue;
            if (d != ALL_DESKTOPS && d != curr_desktop) continue;

            auto vals = toml::table{};

            auto wn = xcb_ewmh_get_wm_name(ewmh, client_list.windows[i]);
            xcb_ewmh_get_utf8_strings_reply_t name_reply;
            if (!xcb_ewmh_get_wm_name_reply(ewmh, wn, &name_reply, nullptr)) continue;

            auto geom = xcb_get_geometry(c, client_list.windows[i]);
            xcb_get_geometry_reply_t *wgem = xcb_get_geometry_reply(c, geom, nullptr);
            if (wgem) {
                vals.insert("geom", GEOM_TO_TOML(wgem));
            }

            std::string window_title(reinterpret_cast<const char*>(name_reply.strings), name_reply.strings_len);
            config.insert(window_title, vals);
            xcb_ewmh_get_utf8_strings_reply_wipe(&name_reply);
        }
        std::ofstream out("scene.toml");
        out << config;
    }
    xcb_ewmh_get_windows_reply_wipe(&client_list);
    return true;
}

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

    capture_desktop(c, &ewmh);


    // auto names = xcb_ewmh_get_desktop_names(&ewmh, screen_n);
    // xcb_ewmh_get_utf8_strings_reply_t names_reply;
    // if (xcb_ewmh_get_utf8_strings_reply(&ewmh, names, &names_reply, nullptr)) {
    //     std::cout << "Desktop names: ";
    //     for (unsigned int i = 0; i < names_reply.strings_len; ++i) {
    //         std::cout << names_reply.strings[i] << '\0';
    //     }
    //     std::cout << std::endl;
    //     xcb_ewmh_get_utf8_strings_reply_wipe(&names_reply);
    // } else {
    //     std::cerr << "Failed to get desktop names." << std::endl;
    // }

    // auto num_of_ds = xcb_ewmh_get_number_of_desktops(&ewmh, screen_n);
    // uint32_t num_of_desktops;
    // if (xcb_ewmh_get_number_of_desktops_reply(&ewmh, num_of_ds, &num_of_desktops, nullptr)){
    //     std::cout << "Number of desktops: " <<  num_of_desktops <<std::endl;
    // }


    // auto cur_ck = xcb_ewmh_get_current_desktop(&ewmh, screen_n);
    // uint32_t cur = 0;
    // if (xcb_ewmh_get_number_of_desktops_reply(&ewmh, cur_ck, &cur, nullptr)){
    //     std::cout << "Current desktop: " << cur + 1 <<std::endl;
    // }

    // auto wa_cookie = xcb_ewmh_get_workarea(&ewmh, screen_n);
    // xcb_ewmh_get_workarea_reply_t workarea_reply;
    // if (xcb_ewmh_get_workarea_reply(&ewmh, wa_cookie, &workarea_reply, nullptr)) {
    //     std::cout << "_NET_WORKAREA values: ";
    //     for (unsigned int i = 0; i < workarea_reply.workarea_len; ++i) {
    //         std::cout << "[" << workarea_reply.workarea[i].x << ", "
    //                   << workarea_reply.workarea[i].y << ", "
    //                   << workarea_reply.workarea[i].width << ", "
    //                   << workarea_reply.workarea[i].height << "] ";
    //     }
    //     std::cout << std::endl;
    //     xcb_ewmh_get_workarea_reply_wipe(&workarea_reply);
    // } else {
    //     std::cerr << "Failed to get _NET_WORKAREA." << std::endl;
    // }

    // auto nc_cookie = xcb_ewmh_get_client_list(&ewmh, screen_n);
    // xcb_ewmh_get_windows_reply_t client_list;
    // if (xcb_ewmh_get_client_list_reply(&ewmh, nc_cookie, &client_list, nullptr)){
    //     std::cout << "Number of windows: " << client_list.windows_len << std::endl;
    //     for (unsigned int i = 0; i < client_list.windows_len; ++i){
    //         std::cout << "\n\nWindow " << i << std::endl;
    //         auto prop_cookie = xcb_get_property(c, 0, client_list.windows[i], ewmh._NET_WM_NAME, XCB_GET_PROPERTY_TYPE_ANY, 0, 1000);
    //         auto reply_prop = xcb_get_property_reply(c, prop_cookie, nullptr);

    //         auto sec_prop_cookie = xcb_get_property(c, 0, client_list.windows[i], ewmh._NET_WM_DESKTOP, XCB_GET_PROPERTY_TYPE_ANY, 0, 1000);
    //         uint32_t cur = 0;
    //         if(reply_prop) {
    //             int value_len2 = xcb_get_property_value_length(reply_prop);
    //             printf("value_len2: %d",value_len2);
    //             if (value_len2) {
    //               char* name = (char*)malloc(value_len2 + 1);
    //               if (name) {
    //                 memcpy(name, xcb_get_property_value(reply_prop), value_len2);
    //                 name[value_len2] = '\0';
    //                 printf("\nName: %s", name);
    //                 fflush(stdout);
    //                 free(name);
    //               }
    //             }
    //             if (xcb_ewmh_get_number_of_desktops_reply(&ewmh, sec_prop_cookie, &cur, nullptr)) {
    //                 printf("\nActive desktop: %i", cur + 1);
    //             }
    //             free(reply_prop);
    //         }
    //     }
    // }
    // xcb_ewmh_get_windows_reply_wipe(&client_list);
}

