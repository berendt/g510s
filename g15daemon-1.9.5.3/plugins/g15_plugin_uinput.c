/*
    This file is part of g15daemon.

    g15daemon is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    g15daemon is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with g15daemon; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    (c) 2006-2008 Mike Lampard, Philip Lawatsch, and others
    
    $Revision: 388 $ -  $Date: 2008-01-02 12:57:03 +1030 (Wed, 02 Jan 2008) $ $Author: mlampard $
    
    UINPUT key processing plugin.  receives events and sends keycodes to the linux kernel.
*/
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <config.h>
#include <g15daemon.h>
#include <pwd.h>

#ifdef HAVE_CONFIG_H
#ifdef HAVE_LINUX_UINPUT_H
#include <linux/input.h>
#include <linux/uinput.h>

#include <libg15.h>

static int uinp_fd = -1;
static config_section_t *uinput_cfg=NULL;
static int map_Lkeys = 0;
static int vol_state = 0;
static int mkey_state = 0;
static int mr_state = 0;
static unsigned int mled_state = G15_LED_M1;
/*mkey state 0,1 and 2 = M1, M2 and M3*/


#define GKEY_OFFSET 167
#define MKEY_OFFSET 185
#define LKEY_OFFSET 189

#define G15KEY_DOWN 1
#define G15KEY_UP 0

static int g15_init_uinput(void *plugin_args) {
    
    int i=0;
    char *custom_filename;
    g15daemon_t *masterlist = (g15daemon_t*) plugin_args;
    struct uinput_user_dev uinp;
    static const char *uinput_device_fn[] = { "/dev/uinput", "/dev/input/uinput","/dev/misc/uinput",0};
    
    uinput_cfg = g15daemon_cfg_load_section(masterlist,"Keyboard OS Mapping (uinput)");
    custom_filename = g15daemon_cfg_read_string(uinput_cfg, "device",(char*)uinput_device_fn[1]);
    map_Lkeys=g15daemon_cfg_read_int(uinput_cfg, "Lkeys.mapped",0);
    
    seteuid(0);
    setegid(0);
    while (uinput_device_fn[i] && (uinp_fd = open(uinput_device_fn[i],O_RDWR))<0){
        ++i;
    }
    if(uinp_fd<0) {	/* try reading the users preference in the config */
        uinp_fd = open(custom_filename,O_RDWR);
    }
    if(uinp_fd<0){
        g15daemon_log(LOG_ERR,"Unable to open UINPUT device.  Please ensure the uinput driver is loaded into the kernel and that you have permission to open the device.");
        return -1;
    }
    /* all other processes/threads should be seteuid nobody */
     seteuid(masterlist->nobody->pw_uid);
     setegid(masterlist->nobody->pw_gid);
    
    
    memset(&uinp,0,sizeof(uinp));
    strncpy(uinp.name, "G15 Extra Keys", UINPUT_MAX_NAME_SIZE);

#ifdef HAVE_UINPUT_USER_DEV_ID
    uinp.id.version = 4;
    uinp.id.bustype = BUS_USB;
#else
    uinp.idversion = 4;
    uinp.idbus = BUS_USB;
#endif 

    ioctl(uinp_fd, UI_SET_EVBIT, EV_KEY);

    for (i=0; i<256; ++i)
        ioctl(uinp_fd, UI_SET_KEYBIT, i);

    write(uinp_fd, &uinp, sizeof(uinp));
    
    if (ioctl(uinp_fd, UI_DEV_CREATE))
    {
        g15daemon_log(LOG_ERR,"Unable to create UINPUT device.");
        return -1;
    }
    return 0;
}

void g15_exit_uinput(void *plugin_args){
    ioctl(uinp_fd, UI_DEV_DESTROY);
    close(uinp_fd);
}


static void g15_uinput_keydown(unsigned char code)
{
    struct input_event event;
    memset(&event, 0, sizeof(event));

    event.type = EV_KEY;
    event.code = code;
    event.value = G15KEY_DOWN;
    
    write (uinp_fd, &event, sizeof(event));

    /* Need to write sync event */
    memset(&event, 0, sizeof(event));

    event.type = EV_SYN;
    event.code = SYN_REPORT;
    event.value = 0;

    write(uinp_fd, &event, sizeof(event));

}

static void g15_uinput_keyup(unsigned char code)
{
    struct input_event event;
    memset(&event, 0, sizeof(event));

    event.type = EV_KEY;
    event.code = code;
    event.value = G15KEY_UP;
    
    write (uinp_fd, &event, sizeof(event));

    /* Need to write sync event */
    memset(&event, 0, sizeof(event));

    event.type = EV_SYN;
    event.code = SYN_REPORT;
    event.value = 0;

    write(uinp_fd, &event, sizeof(event));

}
    void (*keyup)(unsigned char code) = &g15_uinput_keyup;
    void (*keydown)(unsigned char code) = &g15_uinput_keydown;
#else
    void keyup(unsigned char code) { printf("Extra Keys not supported due to missing Uinput.h\n"); }
    void keydown(unsigned char code) { printf("Extra Keys not supported due to missing Uinput.h\n"); }
#endif
#endif
    
/*The layout of the 'G' keys is now hard-coded here below. See /usr/include/linux/input.h for details on the keys you can use*/
static void g15_process_keys(g15daemon_t *masterlist, unsigned int currentkeys, unsigned int lastkeys)
{
    if(!(currentkeys & G15_KEY_LIGHT))
    {
    /*Note: ALL system(""); commands will be run as root as g15daemon needs to be activated by root. Use "su USERNAME -c command" to avoid it.
    also if a command is run from terminal that would post output into the terminal the ability to run commands will hang. to avoid that add "& disown" to command*/
    /*Note2: There's a bug with detecting key release; this means that sometimes if you use key-release function to release the keys, the key will be "stuck" down until you press it again
    that's why I only use the key-release functionality for macros where I need to be able to hold down the button (like tab-switching)*/
    switch(mkey_state)
    {
        //M1 and the default layout (This one i use for commands while in X.
        case 0:
        {
/*M1*/      /*G Keys 1-6 open folders i use all the time.*/
/*M1*/      if((currentkeys & G15_KEY_G1) && !(lastkeys & G15_KEY_G1))
/*M1*/          system("sudo -H -u '#1000' dbus-launch thunar /mnt/Storage/Anime & disown");
/*M1*/
/*M1*/      if((currentkeys & G15_KEY_G2) && !(lastkeys & G15_KEY_G2))
/*M1*/          system("sudo -H -u '#1000' dbus-launch thunar /mnt/Storage/Biomyndir & disown");
/*M1*/
/*M1*/      if((currentkeys & G15_KEY_G3) && !(lastkeys & G15_KEY_G3))
/*M1*/          system("sudo -H -u '#1000' dbus-launch thunar /mnt/Storage/Music & disown");
/*M1*/
/*M1*/      if((currentkeys & G15_KEY_G4) && !(lastkeys & G15_KEY_G4))
/*M1*/          system("sudo -H -u '#1000' dbus-launch thunar /mnt/Storage/Downloads & disown");
/*M1*/
/*M1*/      if((currentkeys & G15_KEY_G5) && !(lastkeys & G15_KEY_G5))
/*M1*/          system("sudo -H -u '#1000' dbus-launch thunar /mnt/Storage/Art & disown");
/*M1*/
/*M1*/      if((currentkeys & G15_KEY_G6) && !(lastkeys & G15_KEY_G6))
/*M1*/          system("sudo -H -u '#1000' dbus-launch thunar $HOME/ & disown");
/*M1*/
/*M1*/
/*M1*/          /*G Keys 7-12* Switch workspaces 1-6 with Win(META)+F1-F6 keys.*/
/*M1*/      if((currentkeys & G15_KEY_G7) && !(lastkeys & G15_KEY_G7))
/*M1*/          {keydown(KEY_LEFTMETA);keydown(KEY_F1);keyup(KEY_F1);keyup(KEY_LEFTMETA);}
/*M1*/      
/*M1*/      if((currentkeys & G15_KEY_G8) && !(lastkeys & G15_KEY_G8))
/*M1*/          {keydown(KEY_LEFTMETA);keydown(KEY_F2);keyup(KEY_F2);keyup(KEY_LEFTMETA);}
/*M1*/
/*M1*/      if((currentkeys & G15_KEY_G9) && !(lastkeys & G15_KEY_G9))
/*M1*/          {keydown(KEY_LEFTMETA);keydown(KEY_F3);keyup(KEY_F3);keyup(KEY_LEFTMETA);}
/*M1*/
/*M1*/      if((currentkeys & G15_KEY_G10) && !(lastkeys & G15_KEY_G10))
/*M1*/          {keydown(KEY_LEFTMETA);keydown(KEY_F4);keyup(KEY_F4);keyup(KEY_LEFTMETA);}
/*M1*/
/*M1*/      if((currentkeys & G15_KEY_G11) && !(lastkeys & G15_KEY_G11))
/*M1*/          {keydown(KEY_LEFTMETA);keydown(KEY_F5);keyup(KEY_F5);keyup(KEY_LEFTMETA);}
/*M1*/
/*M1*/      if((currentkeys & G15_KEY_G12) && !(lastkeys & G15_KEY_G12))
/*M1*/          {keydown(KEY_LEFTMETA);keydown(KEY_F6);keyup(KEY_F6);keyup(KEY_LEFTMETA);}
/*M1*/
/*M1*/
/*M1*/      /*G Keys 13-18 13(Close Tab) 16(Mouseclick(via terminal)) 14(Alt+F4) 17(Open Terminal) 15 and 18 (Next/Prev tab, Ctrl+PgUp/PgDn)*/
/*M1*/      if((currentkeys & G15_KEY_G13) && !(lastkeys & G15_KEY_G13))
/*M1*/          {keydown(KEY_LEFTCTRL);keydown(KEY_W);keyup(KEY_W);keyup(KEY_LEFTCTRL);}
/*M1*/      
/*M1*/      if((currentkeys & G15_KEY_G14) && !(lastkeys & G15_KEY_G14))
/*M1*/          {keydown(KEY_LEFTALT);keydown(KEY_F4);keyup(KEY_F4);keyup(KEY_LEFTALT);}
/*M1*/
/*M1*/      if((currentkeys & G15_KEY_G15) && !(lastkeys & G15_KEY_G15))
/*M1*/          {keydown(KEY_LEFTCTRL);keydown(KEY_PAGEUP);}
/*M1*/      else if(!(currentkeys & G15_KEY_G15) && (lastkeys & G15_KEY_G15))
/*M1*/          {keyup(KEY_PAGEUP);keyup(KEY_LEFTCTRL);}
/*M1*/
/*M1*/          /*Mouse Click Emulation requires xdotool installed*/
/*M1*/      if((currentkeys & G15_KEY_G16) && !(lastkeys & G15_KEY_G16))
/*M1*/          system("sudo -H -u '#1000' xdotool mousedown 1");
/*M1*/      else if(!(currentkeys & G15_KEY_G16) && (lastkeys & G15_KEY_G16))
/*M1*/          system("sudo -H -u '#1000' xdotool mouseup 1");
/*M1*/
/*M1*/      if((currentkeys & G15_KEY_G17) && !(lastkeys & G15_KEY_G17))
/*M1*/          system("sudo -H -u '#1000' terminator --working-directory=~$HOME & disown");
/*M1*/      
/*M1*/      if((currentkeys & G15_KEY_G18) && !(lastkeys & G15_KEY_G18))
/*M1*/          {keydown(KEY_LEFTCTRL);keydown(KEY_PAGEDOWN);}
/*M1*/      else if(!(currentkeys & G15_KEY_G18) && (lastkeys & G15_KEY_G18))
/*M1*/          {keyup(KEY_PAGEDOWN);keyup(KEY_LEFTCTRL);}
        /*M1 END*/
        
        break;
        }

        //M2 layout, This one i use for terminal shortcuts
        case 1:
        {
        /*G Keys 1-6 
        G1((?)) G2(Levels(CTRL+L)) G3(Color Selector(Shift+I)) 
        G4(?) G5(HSL/HSV(CTRL+U)) G6(Shade Selector (Shift+M))
        */
/*M2*/      if((currentkeys & G15_KEY_G1) && !(lastkeys & G15_KEY_G1))
/*M2*/          {keydown(KEY_LEFTCTRL); keydown(KEY_LEFTSHIFT); keydown(KEY_F);keyup(KEY_LEFTCTRL); keyup(KEY_LEFTSHIFT); keyup(KEY_F);}
/*M2*/      
/*M2*/      if((currentkeys & G15_KEY_G2) && !(lastkeys & G15_KEY_G2))
/*M2*/          {keydown(KEY_LEFTCTRL); keydown(KEY_L); keyup(KEY_LEFTCTRL); keyup(KEY_L);}
/*M2*/      
/*M2*/      if((currentkeys & G15_KEY_G3) && !(lastkeys & G15_KEY_G3))
/*M2*/          {keydown(KEY_LEFTSHIFT); keydown(KEY_I); keyup(KEY_LEFTSHIFT); keyup(KEY_I);}
/*M2*/      
/*M2*/      if((currentkeys & G15_KEY_G4) && !(lastkeys & G15_KEY_G4))
/*M2*/          {keydown(KEY_LEFTCTRL);keydown(KEY_K);}
/*M2*/      else if(!(currentkeys & G15_KEY_G4) && (lastkeys & G15_KEY_G4))
/*M2*/          {keyup(KEY_K);keyup(KEY_LEFTCTRL);}
/*M2*/      
/*M2*/      if((currentkeys & G15_KEY_G5) && !(lastkeys & G15_KEY_G5))
/*M2*/          {keydown(KEY_LEFTCTRL); keydown(KEY_U); keyup(KEY_LEFTCTRL); keyup(KEY_U);}
/*M2*/      
/*M2*/      if((currentkeys & G15_KEY_G6) && !(lastkeys & G15_KEY_G6))
/*M2*/          {keydown(KEY_LEFTSHIFT);keydown(KEY_M);keyup(KEY_M);keyup(KEY_LEFTSHIFT);}
/*M2*/
/*M2*/
/*M2*/          /*G Keys 7-12, 
        G7 (Reset Rotation/Canvas View(CTRL+')) G8 (Flip Canvas Vert(Ctrl+M)) G9(Move Layer(T)) 
        G10 (Merge Down (Ctrl+E)) G11 (Flip Canvas Horizontally(M)) G12 (Transform Layer (CTRL+T))
        */
/*M2*/      if((currentkeys & G15_KEY_G7) && !(lastkeys & G15_KEY_G7))
/*M2*/          {keydown(KEY_LEFTCTRL);keydown(KEY_APOSTROPHE);keyup(KEY_LEFTCTRL);keyup(KEY_APOSTROPHE);}
/*M2*/      
/*M2*/      if((currentkeys & G15_KEY_G8) && !(lastkeys & G15_KEY_G8))
/*M2*/          {keydown(KEY_LEFTCTRL);keydown(KEY_M);keyup(KEY_M);keyup(KEY_LEFTCTRL);}
/*M2*/
/*M2*/      if((currentkeys & G15_KEY_G9) && !(lastkeys & G15_KEY_G9))
/*M2*/          {keydown(KEY_T);keyup(KEY_T);}
/*M2*/
/*M2*/      if((currentkeys & G15_KEY_G10) && !(lastkeys & G15_KEY_G10))
/*M2*/          {keydown(KEY_LEFTCTRL);keydown(KEY_E);keyup(KEY_E);keyup(KEY_LEFTCTRL);}
/*M2*/
/*M2*/      if((currentkeys & G15_KEY_G11) && !(lastkeys & G15_KEY_G11))
/*M2*/          {keydown(KEY_M);keyup(KEY_M);}
/*M2*/
/*M2*/      if((currentkeys & G15_KEY_G12) && !(lastkeys & G15_KEY_G12))
/*M2*/          {keydown(KEY_LEFTCTRL);keydown(KEY_T);keyup(KEY_T);keyup(KEY_LEFTCTRL);}
/*M2*/
/*M2*/
/*M2*/      /*G Keys 13-18, 
        G13(Freehand Select (?)) G14(Increase Opacity of Brush(I)) G15(Darker Color(K))
        G16(Deselect (CTRL+D) G17(Decrease Opacity of Brush(O)) G18(Lighter color (L))
        */
/*M2*/      if((currentkeys & G15_KEY_G13) && !(lastkeys & G15_KEY_G13))
/*M2*/          {keydown(KEY_LEFTCTRL);keydown(KEY_LEFTSHIFT);keydown(KEY_F);keyup(KEY_F);keyup(KEY_LEFTSHIFT);keyup(KEY_LEFTCTRL);}
/*M2*/      
/*M2*/      if((currentkeys & G15_KEY_G14) && !(lastkeys & G15_KEY_G14))
/*M2*/          {keydown(KEY_I);}
/*M2*/      else if(!(currentkeys & G15_KEY_G14) && (lastkeys & G15_KEY_G14))
/*M2*/          {keyup(KEY_I);}
/*M2*/
/*M2*/      if((currentkeys & G15_KEY_G15) && !(lastkeys & G15_KEY_G15))
/*M2*/          {keydown(KEY_K);}
/*M2*/      else if(!(currentkeys & G15_KEY_G15) && (lastkeys & G15_KEY_G15))
/*M2*/          {keyup(KEY_K);}
/*M2*/      
/*M2*/      if((currentkeys & G15_KEY_G16) && !(lastkeys & G15_KEY_G16))
/*M2*/          {keydown(KEY_LEFTCTRL);keydown(KEY_D); keyup(KEY_LEFTCTRL); keyup(KEY_D);}
/*M2*/      
/*M2*/      if((currentkeys & G15_KEY_G17) && !(lastkeys & G15_KEY_G17))
/*M2*/          {keydown(KEY_O);}
/*M2*/      else if(!(currentkeys & G15_KEY_G17) && (lastkeys & G15_KEY_G17))
/*M2*/          {keyup(KEY_O);}
/*M2*/      
/*M2*/      if((currentkeys & G15_KEY_G18) && !(lastkeys & G15_KEY_G18))
/*M2*/          {keydown(KEY_L);}
/*M2*/      else if(!(currentkeys & G15_KEY_G18) && (lastkeys & G15_KEY_G18))
/*M2*/          {keyup(KEY_L);}
        /*M2 END*/
        break;
        }
        //M3 layout for terminal commands (Placeholder)
        case 2:
        {
        /*G Keys 1-6*/
        /*G keys 1-6 G1(EraseLineLeft) G2(EraseWordLeft) G3(WordLeft) G4(EraseLineRight) G5(EraseWordRight) G6(WordRight)*/
/*M3*/      if((currentkeys & G15_KEY_G1) && !(lastkeys & G15_KEY_G1))
/*M3*/          {keydown(KEY_LEFTCTRL); keydown(KEY_U);}
/*M3*/      else if(!(currentkeys & G15_KEY_G1) && (lastkeys & G15_KEY_G1))
/*M3*/          {keyup(KEY_U);keyup(KEY_LEFTCTRL);}
/*M3*/      
/*M3*/      if((currentkeys & G15_KEY_G2) && !(lastkeys & G15_KEY_G2))
/*M3*/          {keydown(KEY_LEFTCTRL);keydown(KEY_W);}
/*M3*/      else if(!(currentkeys & G15_KEY_G2) && (lastkeys & G15_KEY_G2))
/*M3*/          {keyup(KEY_W);keyup(KEY_LEFTCTRL);}
/*M3*/      
/*M3*/      if((currentkeys & G15_KEY_G3) && !(lastkeys & G15_KEY_G3))
/*M3*/          {keydown(KEY_LEFTALT);keydown(KEY_B);}
/*M3*/      else if(!(currentkeys & G15_KEY_G3) && (lastkeys & G15_KEY_G3))
/*M3*/          {keyup(KEY_B);keyup(KEY_LEFTALT);}
/*M3*/      
/*M3*/      if((currentkeys & G15_KEY_G4) && !(lastkeys & G15_KEY_G4))
/*M3*/          {keydown(KEY_LEFTCTRL);keydown(KEY_K);}
/*M3*/      else if(!(currentkeys & G15_KEY_G4) && (lastkeys & G15_KEY_G4))
/*M3*/          {keyup(KEY_K);keyup(KEY_LEFTCTRL);}
/*M3*/      
/*M3*/      if((currentkeys & G15_KEY_G5) && !(lastkeys & G15_KEY_G5))
/*M3*/          {keydown(KEY_LEFTALT);keydown(KEY_D);}
/*M3*/      else if(!(currentkeys & G15_KEY_G5) && (lastkeys & G15_KEY_G5))
/*M3*/          {keyup(KEY_D);keyup(KEY_LEFTALT);}
/*M3*/      
/*M3*/      if((currentkeys & G15_KEY_G6) && !(lastkeys & G15_KEY_G6))
/*M3*/          {keydown(KEY_LEFTALT);keydown(KEY_F);}
/*M3*/      else if(!(currentkeys & G15_KEY_G6) && (lastkeys & G15_KEY_G6))
/*M3*/          {keyup(KEY_F);keyup(KEY_LEFTALT);}
/*M3*/
/*M3*/
/*M3*/          /*G Keys 7-12, Switch between terms 1-6 with Ctrl+Alt+F1-F6*/
/*M3*/      if((currentkeys & G15_KEY_G7) && !(lastkeys & G15_KEY_G7))
/*M3*/          {keydown(KEY_LEFTCTRL);keydown(KEY_LEFTALT);keydown(KEY_F1);}
/*M3*/      else if(!(currentkeys & G15_KEY_G7) && (lastkeys & G15_KEY_G7))
/*M3*/          {keyup(KEY_F1);keyup(KEY_LEFTALT);keyup(KEY_LEFTCTRL);}
/*M3*/      
/*M3*/      if((currentkeys & G15_KEY_G8) && !(lastkeys & G15_KEY_G8))
/*M3*/          {keydown(KEY_LEFTCTRL);keydown(KEY_LEFTALT);keydown(KEY_F2);}
/*M3*/      else if(!(currentkeys & G15_KEY_G8) && (lastkeys & G15_KEY_G8))
/*M3*/          {keyup(KEY_F2);keyup(KEY_LEFTALT);keyup(KEY_LEFTCTRL);}
/*M3*/
/*M3*/      if((currentkeys & G15_KEY_G9) && !(lastkeys & G15_KEY_G9))
/*M3*/          {keydown(KEY_LEFTCTRL);keydown(KEY_LEFTALT);keydown(KEY_F3);}
/*M3*/      else if(!(currentkeys & G15_KEY_G9) && (lastkeys & G15_KEY_G9))
/*M3*/          {keyup(KEY_F3);keyup(KEY_LEFTALT);keyup(KEY_LEFTCTRL);}
/*M3*/
/*M3*/      if((currentkeys & G15_KEY_G10) && !(lastkeys & G15_KEY_G10))
/*M3*/          {keydown(KEY_LEFTCTRL);keydown(KEY_LEFTALT);keydown(KEY_F4);}
/*M3*/      else if(!(currentkeys & G15_KEY_G10) && (lastkeys & G15_KEY_G10))
/*M3*/          {keyup(KEY_F4);keyup(KEY_LEFTALT);keyup(KEY_LEFTCTRL);}
/*M3*/
/*M3*/      if((currentkeys & G15_KEY_G11) && !(lastkeys & G15_KEY_G11))
/*M3*/          {keydown(KEY_LEFTCTRL);keydown(KEY_LEFTALT);keydown(KEY_F5);}
/*M3*/      else if(!(currentkeys & G15_KEY_G11) && (lastkeys & G15_KEY_G11))
/*M3*/          {keyup(KEY_F5);keyup(KEY_LEFTALT);keyup(KEY_LEFTCTRL);}
/*M3*/
/*M3*/      if((currentkeys & G15_KEY_G12) && !(lastkeys & G15_KEY_G12))
/*M3*/          {keydown(KEY_LEFTCTRL);keydown(KEY_LEFTALT);keydown(KEY_F6);}
/*M3*/      else if(!(currentkeys & G15_KEY_G12) && (lastkeys & G15_KEY_G12))
/*M3*/          {keyup(KEY_F6);keyup(KEY_LEFTALT);keyup(KEY_LEFTCTRL);}
/*M3*/
/*M3*/
/*M3*/      /*G Keys 13-18 13(Switch to GUI) 16(Kill X11) 14(Ctrl+C) 17(Q+Enter) 15/18(Shift+PgUp/PgDn for Scroll Up/Dn)*/
/*M3*/      if((currentkeys & G15_KEY_G13) && !(lastkeys & G15_KEY_G13))
/*M3*/          {keydown(KEY_LEFTCTRL);keydown(KEY_LEFTALT);keydown(KEY_F7);}
/*M3*/      else if(!(currentkeys & G15_KEY_G13) && (lastkeys & G15_KEY_G13))
/*M3*/          {keyup(KEY_F7);keyup(KEY_LEFTALT);keyup(KEY_LEFTCTRL);}
/*M3*/      
/*M3*/      if((currentkeys & G15_KEY_G14) && !(lastkeys & G15_KEY_G14))
/*M3*/          {keydown(KEY_LEFTCTRL);keydown(KEY_C);}
/*M3*/      else if(!(currentkeys & G15_KEY_G14) && (lastkeys & G15_KEY_G14))
/*M3*/          {keyup(KEY_C);keyup(KEY_LEFTCTRL);}
/*M3*/
/*M3*/      if((currentkeys & G15_KEY_G15) && !(lastkeys & G15_KEY_G15))
/*M3*/          {keydown(KEY_LEFTSHIFT);keydown(KEY_PAGEUP);}
/*M3*/      else if(!(currentkeys & G15_KEY_G15) && (lastkeys & G15_KEY_G15))
/*M3*/          {keyup(KEY_PAGEUP);keyup(KEY_LEFTSHIFT);}
/*M3*/      
/*M3*/      if((currentkeys & G15_KEY_G16) && !(lastkeys & G15_KEY_G16))
/*M3*/          {keydown(KEY_LEFTCTRL);keydown(KEY_LEFTALT);keydown(KEY_BACKSPACE);}
/*M3*/      else if(!(currentkeys & G15_KEY_G16) && (lastkeys & G15_KEY_G16))
/*M3*/          {keyup(KEY_BACKSPACE);keyup(KEY_LEFTALT);keyup(KEY_LEFTCTRL);}
/*M3*/      
/*M3*/      if((currentkeys & G15_KEY_G17) && !(lastkeys & G15_KEY_G17))
/*M3*/          {keydown(KEY_Q);}
/*M3*/      else if(!(currentkeys & G15_KEY_G17) && (lastkeys & G15_KEY_G17))
/*M3*/          {keyup(KEY_Q);keydown(KEY_ENTER);keyup(KEY_ENTER);}
/*M3*/      
/*M3*/      if((currentkeys & G15_KEY_G18) && !(lastkeys & G15_KEY_G18))
/*M3*/          {keydown(KEY_LEFTSHIFT);keydown(KEY_PAGEDOWN);}
/*M3*/      else if(!(currentkeys & G15_KEY_G18) && (lastkeys & G15_KEY_G18))
/*M3*/          {keyup(KEY_PAGEDOWN);keyup(KEY_LEFTSHIFT);}
        /*M3 END*/
        break;
        } 
    }
    
        /* 'M' keys... */
    
        if((currentkeys & G15_KEY_M1) && !(lastkeys & G15_KEY_M1))
            //keydown(MKEY_OFFSET);
        mkey_state = 0;
        /*else if(!(currentkeys & G15_KEY_M1) && (lastkeys & G15_KEY_M1))
            keyup(MKEY_OFFSET);*/
    
        if((currentkeys & G15_KEY_M2) && !(lastkeys & G15_KEY_M2))
            //keydown(MKEY_OFFSET+1);
        mkey_state = 1;
        /*else if(!(currentkeys & G15_KEY_M2) && (lastkeys & G15_KEY_M2))
            keyup(MKEY_OFFSET+1);*/
    
        if((currentkeys & G15_KEY_M3) && !(lastkeys & G15_KEY_M3))
            //keydown(MKEY_OFFSET+2);
        mkey_state = 2;
        /*else if(!(currentkeys & G15_KEY_M3) && (lastkeys & G15_KEY_M3))
            keyup(MKEY_OFFSET+2);*/
    
    if((currentkeys & G15_KEY_MR) && !(lastkeys & G15_KEY_MR) && mr_state == 0){
        mr_state = 1;
    }
    else if(!(currentkeys & G15_KEY_MR) && (lastkeys & G15_KEY_MR) && mr_state == 1){
    /*Toggle function on first key release*/
    }
    else if((currentkeys & G15_KEY_MR) && !(lastkeys & G15_KEY_MR) && mr_state == 1){
        mr_state = 0;
    }
    else if(!(currentkeys & G15_KEY_MR) && (lastkeys & G15_KEY_MR) && mr_state == 0){
    /*Úntoggle function on second key release*/
    }
        
        if(map_Lkeys){
            /* 'L' keys...  */
            if((currentkeys & G15_KEY_L1) && !(lastkeys & G15_KEY_L1))
                keydown(LKEY_OFFSET);
            else if(!(currentkeys & G15_KEY_L1) && (lastkeys & G15_KEY_L1))
                keyup(LKEY_OFFSET);
    
            if((currentkeys & G15_KEY_L2) && !(lastkeys & G15_KEY_L2))
                keydown(LKEY_OFFSET+1);
            else if(!(currentkeys & G15_KEY_L2) && (lastkeys & G15_KEY_L2))
                keyup(LKEY_OFFSET+1);
    
            if((currentkeys & G15_KEY_L3) && !(lastkeys & G15_KEY_L3))
                keydown(LKEY_OFFSET+2);
            else if(!(currentkeys & G15_KEY_L3) && (lastkeys & G15_KEY_L3))
                keyup(LKEY_OFFSET+2);
    
            if((currentkeys & G15_KEY_L4) && !(lastkeys & G15_KEY_L4))
                keydown(LKEY_OFFSET+3);
            else if(!(currentkeys & G15_KEY_L4) && (lastkeys & G15_KEY_L4))
                keyup(LKEY_OFFSET+3);
    
            if((currentkeys & G15_KEY_L5) && !(lastkeys & G15_KEY_L5))
                keydown(LKEY_OFFSET+4);
            else if(!(currentkeys & G15_KEY_L5) && (lastkeys & G15_KEY_L5))
                keyup(LKEY_OFFSET+4);
        }

    }


     else
     {
        // G15_KEY_LIGHT - Key modifier for Logitech G510 Media Keys implementation

        // XF86AudioPlay
        if((currentkeys & G15_KEY_G1) && !(lastkeys & G15_KEY_G1))
            keydown(KEY_PLAYPAUSE);
        else if(!(currentkeys & G15_KEY_G1) && (lastkeys & G15_KEY_G1))
            keyup(KEY_PLAYPAUSE);

        // XF86AudioStop
        if((currentkeys & G15_KEY_G2) && !(lastkeys & G15_KEY_G2))
            keydown(KEY_STOPCD);
        else if(!(currentkeys & G15_KEY_G2) && (lastkeys & G15_KEY_G2))
            keyup(KEY_STOPCD);

        // XF86AudioPrev
        if((currentkeys & G15_KEY_G3) && !(lastkeys & G15_KEY_G3))
            keydown(KEY_PREVIOUSSONG);
        else if(!(currentkeys & G15_KEY_G3) && (lastkeys & G15_KEY_G3))
            keyup(KEY_PREVIOUSSONG);

        // XF86AudioNext
        if((currentkeys & G15_KEY_G4) && !(lastkeys & G15_KEY_G4))
            keydown(KEY_NEXTSONG);
        else if(!(currentkeys & G15_KEY_G4) && (lastkeys & G15_KEY_G4))
            keyup(KEY_NEXTSONG);

        // XF86AudioMute
        if((currentkeys & G15_KEY_G5) && !(lastkeys & G15_KEY_G5))
            keydown(KEY_MUTE);
        else if(!(currentkeys & G15_KEY_G5) && (lastkeys & G15_KEY_G5))
            keyup(KEY_MUTE);

        // XF86AudioRaise/LowerVolume (this was a pain to figure out, i dropped it at one point while it was semi-functioning and fixed it later)
        if(((currentkeys & G15_KEY_G6) && !(lastkeys & G15_KEY_G6)) && (vol_state == 0 || vol_state == 1)){
        keydown(KEY_VOLUMEUP);
        vol_state = 1;
    }
        else if(((currentkeys & G15_KEY_G7) && !(lastkeys & G15_KEY_G7)) && (vol_state == 0 || vol_state == 2)){
        keydown(KEY_VOLUMEDOWN);
        vol_state = 2;
    }
    else if((!(currentkeys & G15_KEY_G6) || !(currentkeys & G15_KEY_G7)) && ((lastkeys & G15_KEY_G7) || (lastkeys & G15_KEY_G6))){ 
        keyup(KEY_VOLUMEUP);
            keyup(KEY_VOLUMEDOWN);
        vol_state = 0;  
        }
    }
/*Set led states per M key (Doing this in the switch was slow and didn't activate till after button release)*/
    if(mkey_state == 0)
    mled_state = G15_LED_M1;
    else if(mkey_state == 1)
    mled_state = G15_LED_M2;
    else if(mkey_state == 2)
    mled_state = G15_LED_M3;

/*Set leds and toggle MR led depending on mr_state*/
    if(mr_state == 0)
    setLEDs(mled_state -0x20);
    else if(mr_state == 1)
    setLEDs(mled_state -0x20 | G15_LED_MR -0x20);

}


static int keyevents(plugin_event_t *myevent) {
    lcd_t *lcd = (lcd_t*) myevent->lcd;
    static int lastkeys;
    switch (myevent->event)
    {
        case G15_EVENT_KEYPRESS:{
            g15_process_keys(lcd->masterlist, myevent->value,lastkeys);
            lastkeys = myevent->value;
            break;
        }
        case G15_EVENT_VISIBILITY_CHANGED:
        case G15_EVENT_USER_FOREGROUND:
	case G15_EVENT_MLED:
        case G15_EVENT_BACKLIGHT:
        case G15_EVENT_CONTRAST:
        case G15_EVENT_REQ_PRIORITY:
        case G15_EVENT_CYCLE_PRIORITY:
        default:
            break;
    }
    return G15_PLUGIN_OK;
}


    /* if no exitfunc or eventhandler, member should be NULL */
plugin_info_t g15plugin_info[] = {
        /* TYPE, name, initfunc, updatefreq, exitfunc, eventhandler, initfunc */
   {G15_PLUGIN_CORE_OS_KB, "Linux UINPUT Keyboard Output"	, NULL, 500, (void*)g15_exit_uinput, (void*)keyevents, (void*)g15_init_uinput},
   {G15_PLUGIN_NONE,               ""          			, NULL,   0,   			NULL,            NULL,           NULL}
};
