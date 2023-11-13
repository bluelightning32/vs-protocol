## Example output

![screenshot](https://raw.githubusercontent.com/bluelightning32/vs-protocol/main/doc/example1.png)

## Supported features

Currently only Linux is supported.

Connections to multiplayer servers use TCP, which Wireshark can capture and decode. It does not work in single player mode. Single player mode uses a dummy connection that does not use TCP, such that Wireshark cannot see its traffic.

Only the outer protobuf is parsed. Some protobufs have inner fields which are serialized with another format, such as protobuf again, TreeAttribute, or JSON. That second layer of serialization is not dissected. It just shows up as a byte array.

## Installation directories

Wireshark has some standard named folders. The actual location of these folders vary in different distros. The exact locations can be found in the folders tab of the About Wireshark dialog in the Help menu. Below is an example from Fedora 38.
![screenshot](https://raw.githubusercontent.com/bluelightning32/vs-protocol/main/doc/wireshark_folders.png)

The final files consist of a shared library plugin (`libvintage_story.so`) and a Protocol Buffer definition (`vintage_story.proto`). These files may either be installed in a global location for all users (requires root access), or in a directory for one user (does not require root access).

* Global installation
    * Install `libvintage_story.so` in the "Global Plugins" folder (typically /usr/lib64/wireshark/plugins/4.0).
    * Install `vintage_story.proto` in the "Global configuration" folder (typically /usr/share/wireshark).
* User installation
    * Install `libvintage_story.so` in the "Personal Plugins" folder (typically ~/.local/lib/wireshark/plugins/4.0). Create the folder if it does not exist yet.
    * Install `vintage_story.proto` in the "Global configuration" folder (typically ~/.config/wireshark). Create the folder if it does not exist yet.

If it worked, the plugin will be listed on the Plugins tab of the About dialog.
![screenshot](https://raw.githubusercontent.com/bluelightning32/vs-protocol/main/doc/wireshark_plugins.png)

## Dissecting traffic on a non-default VS server port

The plugin will automatically recognize TCP traffic to VS servers running on the default 42420 port. For servers running on other ports, first capture the data. Then select a packet from the server to the client (do not use a packet from the client to the server), then click 'Decode As...'. Double click the Current column, and select VintageStory in the drop down. Click Save then OK.
![screenshot](https://raw.githubusercontent.com/bluelightning32/vs-protocol/main/doc/decode_as.png)

## Decoding large packets

At the network layer, large packets get fragmented into smaller packets. The vs-protocol dissector relies on Wireshark's built-in TCP dissector to reassemble those fragments so that they can be decoded as a full VS packet. For very large packets, such as the ServerAssets packet, those fragments often get reordered on the network. By default the TCP dissector does not handle reordered fragments. It is advised to turn on the 'Reassemble out-of-order segments' so that it does handle that, so that the vs-protocol dissector and decode those very large packets.

The setting can be found in the Wireshark Preferences dialog, under the 'Protocols > TCP' tree.
![screenshot](https://raw.githubusercontent.com/bluelightning32/vs-protocol/main/doc/tcp_preferences.png)

Those very large packets add more Protobuf fields to the tree than Wireshark supports by default. This shows up as a warning 'Adding protobuf.field.number would put more than xxx items in the tree -- possible infinite loop' in the protocol tree. This limit can be increased through the gui.max\_tree\_items preference, but doing so does seem to make the GUI less responsive on the large packets.
![screenshot](https://raw.githubusercontent.com/bluelightning32/vs-protocol/main/doc/max_tree_elements.png)

## Building

Building requires the following to be installed:
* A C++ compiler
* The Wireshark-dev package
* The libzstd-devel package
* CMake

First create a build directory, typically as a subdirectory inside of the source checkout, and enter it.
```
mkdir build
cd build
```

Run CMake to configure the plugin. With the default options, the plugin will be installed in the global location (requires root).
```
cmake ../
```

Alternatively, options can be passed to CMake to install in the user directories, which does not require root.
```
cmake ../ -DPLUGIN_DIR:STRING=~/.local/lib/wireshark/plugins/4.0 -DPROTOBUF_DIR:STRING=~/.config/wireshark/protobuf
```

Finally build the dissector and install it.
```
make install
```

## Notes on dependencies

Vintage Story uses ZStandard to compress large packets. So the dissector must be able to decompress ZStandard. Wireshark 4.2 has builtin support for zstd, but 4.2 is still labelled as a developer release. The stable version, 4.0, does not export a ZStandard function to plugins. So instead vs-protocol links directly against libzstd.

Wireshark supports writing plugins in LUA, which is more portable than C/C++. However, even in the Wireshark development build, the vs-protocol plugin cannot be written in LUA, because Wireshark does not expose all of the compression functions in the LUA API ([issue 16451](https://gitlab.com/wireshark/wireshark/-/issues/16451)). Furthermore, Wireshark does not include any way for LUA to call arbitrary native APIs. Theoretically one could rewrite zstd in LUA, but that would be very difficult.

This dissector is GPLv2 licensed, because all Wireshark plugins [must be](https://wiki.wireshark.org/Lua#beware-the-gpl) GPLv2 licensed, even LUA dissectors. Furthermore, the Vintage Story Protobuf file was contributed by Tyron with the understanding that it would be GPLv2 licensed.
