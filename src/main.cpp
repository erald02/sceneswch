#include "fmt/base.h"
#include <cstddef>
#include <cstring>
#include <iterator>
#include <string>
#include <xcb/xproto.h>
extern "C" {
#include <xcb/xcb.h>
#include <xcb/randr.h>
#include <xcb/xcb_ewmh.h>
}
#include <iostream>
#include <toml++/toml.hpp>

int SCREEN_N = 0;
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
    auto ck = xcb_ewmh_get_current_desktop(ewmh, SCREEN_N);
    uint32_t cur = 0;
    if (!xcb_ewmh_get_current_desktop_reply(ewmh, ck, &cur, nullptr)) return -1;
    return static_cast<int>(cur);
}

// std::string p_from_pid(int pid){

// }

#define GEOM_TO_TOML(g) toml::table{ \
    {"x", g->x}, {"y", g->y}, {"w", g->width}, {"h", g->height}, \
    {"depth", g->depth}, {"border_width", g->border_width} \
}

#define MON_TO_TOML(g) toml::table{ \
    {"sequence", g->sequence}, {"length", g->length}, {"nMonitors", g->nMonitors}, \
    {"nOutputs", g->nOutputs} \
}

// xcb_atom_t name;
// uint8_t    primary;
// uint8_t    automatic;
// uint16_t   nOutput;
// int16_t    x;
// int16_t    y;
// uint16_t   width;
// uint16_t   height;
// uint32_t   width_in_millimeters;
// uint32_t   height_in_millimeters;

#define O_MON_TO_TOML(g) toml::table{ \
    {"primary", g->primary}, {"x", g->x}, {"y", g->y}, \
    {"width", g->width}, {"height", g->height} \
}


bool capture_desktop(xcb_connection_t *c, xcb_ewmh_connection_t *ewmh) {
    int curr_desktop = get_current_desktop(ewmh);
    toml::array windows;

    auto nc_cookie = xcb_ewmh_get_client_list(ewmh, SCREEN_N);
    xcb_ewmh_get_windows_reply_t client_list;
    if (xcb_ewmh_get_client_list_reply(ewmh, nc_cookie, &client_list, nullptr)){
        for (unsigned int i = 0; i < client_list.windows_len; ++i){
            auto ck = xcb_ewmh_get_wm_desktop(ewmh, client_list.windows[i]);
            uint32_t d = 0;
            if (!xcb_ewmh_get_wm_desktop_reply(ewmh, ck, &d, nullptr)) continue;
            if (d != ALL_DESKTOPS && d != static_cast<uint32_t>(curr_desktop)) continue;
        

            auto wn = xcb_ewmh_get_wm_name(ewmh, client_list.windows[i]);
            xcb_ewmh_get_utf8_strings_reply_t name_reply;
            if (!xcb_ewmh_get_wm_name_reply(ewmh, wn, &name_reply, nullptr)) continue;
            std::string window_title(reinterpret_cast<const char*>(name_reply.strings), name_reply.strings_len);
            xcb_ewmh_get_utf8_strings_reply_wipe(&name_reply);
            toml::table t;
            t.insert("title", window_title);
            t.insert("desktop", static_cast<int64_t>(d));

            auto geom = xcb_get_geometry(c, client_list.windows[i]);
            xcb_get_geometry_reply_t *wgem = xcb_get_geometry_reply(c, geom, nullptr);
            // TODO: translate coords to framed rect as they root relative :/ 
            if (wgem) { t.insert("geom", GEOM_TO_TOML(wgem)); free(wgem); }

            auto x = xcb_randr_get_monitors(c, client_list.windows[i],0);
            auto val = xcb_randr_get_monitors_reply(c, x, nullptr);
            int idx = 0;
            if (val) {
                auto m_iter = xcb_randr_get_monitors_monitors_iterator(val);
                while (m_iter.rem) {
                    t.insert(static_cast<char[]>(idx), O_MON_TO_TOML(*&m_iter.data));
                    // TODO: then find highest overlap and determine monitor
                    xcb_randr_monitor_info_next(&m_iter);
                    idx += 1;
                }
                t.insert("moni", MON_TO_TOML(val));
                free(val);
            }

            windows.push_back(std::move(t));
        }
        toml::table root;
        root.insert("window", std::move(windows));
        std::ofstream out("scene.toml"); out << root;
    }
    xcb_ewmh_get_windows_reply_wipe(&client_list);
    return true;
}

int main() {
    std::cout << "Connecting" << std::endl;
    xcb_connection_t *c = xcb_connect(nullptr, &SCREEN_N);
    if (xcb_connection_has_error(c)) return 1;
    xcb_screen_t *s = xcb_setup_roots_iterator(xcb_get_setup(c)).data;

    xcb_ewmh_connection_t ewmh;
    xcb_intern_atom_cookie_t *cookie = xcb_ewmh_init_atoms(c, &ewmh);
    if (!xcb_ewmh_init_atoms_replies(&ewmh, cookie, nullptr)) {
        std::cerr << "Failed to initialize EWMH atoms" << std::endl;
        return 1;
    }

    capture_desktop(c, &ewmh);
    xcb_disconnect(c);
    return 0;
}

