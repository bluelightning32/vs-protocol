#include <algorithm> // for std::max
#include <iostream>
#include <iterator> // for std::size
#include <limits> // for std::numeric_limits
#include <sstream> // for std::ostringstream
#include <vector>

#include <epan/packet.h> // for create_dissector_handle
#include <epan/proto.h> // for proto_register_protocol
#include <zstd.h>

#include "client_id.h"
#include "server_id.h"

namespace vintage_story {

namespace {
constexpr int kDefaultServerPort = 42420;

int proto_id;
int hf_compressed;
int hf_packet_length;
int hf_compressed_payload;
int hf_padding;
int protocol_ett = -1;
int compressed_payload_ett = -1;

void SetInfoColumn(packet_info *pinfo,
                     const std::vector<std::string> &packet_names) {
  if (packet_names.empty()) {
    // Nothing was successfully decoded. Don't change the info field.
    return;
  }
  std::string joined;
  for (size_t i = 0; i < packet_names.size(); ++i) {
    joined += packet_names[i];
    joined += " ";
  }
  col_add_str(pinfo->cinfo, COL_INFO, joined.c_str());
}

long DecodeVarInt(tvbuff_t *payload, gint &offset) {
  long val = 0;
  bool more = true;
  int shift = 0;
  while (tvb_captured_length_remaining(payload, offset) > 0 && more) {
    unsigned char read = tvb_get_guint8(payload, offset);
    val |= (read & 0x7F) << shift;
    more = read & 0x80;
    ++offset;
    shift += 7;
  }
  return val;
}

gint GetProtoBufEnd(tvbuff_t *payload) {
  if (tvb_captured_length(payload) >
      static_cast<guint>(std::numeric_limits<gint>::max())) {
    std::cerr << "Captured packet size, " << tvb_captured_length(payload)
              << ", is beyond gint range." << std::endl;
    return 0;
  }
  gint offset = 0;
  while (tvb_captured_length_remaining(payload, offset) > 0) {
    gint prev_offset = offset;
    long tag = DecodeVarInt(payload, offset);
    if (tag == 0) {
      offset = prev_offset;
      break;
    }

    int wire_type = tag & 0x7;
    if (wire_type == 0) {
      DecodeVarInt(payload, offset);
    } else if (wire_type == 1) {
      offset += 8;
    } else if (wire_type == 2) {
      offset += DecodeVarInt(payload, offset);
    } else if (wire_type == 5) {
      offset += DecodeVarInt(payload, offset);
    }
  }
  return std::min(offset, static_cast<gint>(tvb_captured_length(payload)));
}

int FindVarInt(tvbuff_t *payload, int find_tag, bool &found) {
  found = false;
  gint offset = 0;
  while (tvb_captured_length_remaining(payload, offset) > 0) {
    long tag = DecodeVarInt(payload, offset);
    if (tag == 0) {
      return -1;
    }

    int wire_type = tag & 0x7;
    if (wire_type == 0) {
      int val = DecodeVarInt(payload, offset);
      if (tag >> 3 == find_tag) {
        found = true;
        return val;
      }
    } else if (wire_type == 1) {
      offset += 8;
    } else if (wire_type == 2) {
      offset += DecodeVarInt(payload, offset);
    } else if (wire_type == 5) {
      offset += DecodeVarInt(payload, offset);
    }
  }
  return -1;
}

std::string DecodePacket(bool is_server, tvbuff_t *payload,
                         packet_info *pinfo, proto_tree *tree) {
  dissector_handle_t protobuf_handle = find_dissector("protobuf");
  std::ostringstream out;
  if (is_server) {
    if (tvb_captured_length(payload) <= 32 &&
        (tvb_captured_length(payload) & 0x1f) == 0) {
      // The serializer pads short messages with nulls to a length that's a
      // multiple of 16. The protobuf dissector prints errors if they are
      // passed to it. So use some heuristics to find the end of the message.
      gint payload_end = GetProtoBufEnd(payload);
      tvbuff_t *unpadded = tvb_new_subset_length(payload, 0, payload_end);
      call_dissector_with_data(
          protobuf_handle, unpadded, pinfo, tree,
          const_cast<void *>(
            reinterpret_cast<const void *>("message,Packet.Server")));
      proto_tree_add_item(tree, hf_padding, payload, /*offset=*/payload_end,
                          tvb_reported_length_remaining(payload, payload_end),
                          ENC_NA);
    } else {
      call_dissector_with_data(
          protobuf_handle, payload, pinfo, tree,
          const_cast<void *>(
            reinterpret_cast<const void *>("message,Packet.Server")));
    }

    bool found = false;
    int id = FindVarInt(payload, 90, found);
    out << "Server ";
    if (found) {
      auto it = server_ids.find(id);
      if (it != server_ids.end()) {
        out << it->second;
      } else {
        out << "id=" << id;
      }
    } else {
      gint offset = 0;
      long first_tag = DecodeVarInt(payload, offset);
      if (first_tag == 10) {
        out << "ServerIdentification";
      } else {
        out << "first_tag=" << first_tag;
      }
    }
  } else {
    if (tvb_captured_length(payload) == 16) {
      // The client serializer pads short messages with nulls up to length 16.
      // The protobuf dissector prints errors if they are passed to it. So use
      // some heuristics to find the end of the message.
      gint payload_end = GetProtoBufEnd(payload);
      tvbuff_t *unpadded = tvb_new_subset_length(payload, 0, payload_end);
      call_dissector_with_data(
          protobuf_handle, unpadded, pinfo, tree,
          const_cast<void *>(
            reinterpret_cast<const void *>("message,Packet.Client")));
      proto_tree_add_item(tree, hf_padding, payload, /*offset=*/payload_end,
                          tvb_reported_length_remaining(payload, payload_end),
                          ENC_NA);
    } else {
      call_dissector_with_data(
          protobuf_handle, payload, pinfo, tree,
          const_cast<void *>(
            reinterpret_cast<const void *>("message,Packet.Client")));
    }
    bool found = false;
    int id = FindVarInt(payload, 1, found);
    out << "Client ";
    if (found) {
      auto it = client_ids.find(id);
      if (it != client_ids.end()) {
        out << it->second;
      } else {
        out << "id=" << id;
      }
    } else {
      gint offset = 0;
      long first_tag = DecodeVarInt(payload, offset);
      if (first_tag == 18) {
        out << "ClientIdentification";
      } else {
        out << "first_tag=" << first_tag;
      }
    }
  }
  return out.str();
}

gint Dissect(tvbuff_t *buffer, packet_info *pinfo, proto_tree *tree,
            void* /*data*/) {
  if (tvb_captured_length(buffer) < tvb_reported_length(buffer)) {
    // Don't attempt to decode packets that were truncated because the snapshot
    // length (-s option to tcpdump) was set too low.
    col_set_str(pinfo->cinfo, COL_PROTOCOL, "VintageStory");
    col_set_str(pinfo->cinfo, COL_INFO, "Truncated due to snap length");
    std::cerr << "Rejecting truncated packet" << std::endl;
    return 0;
  }

  col_set_str(pinfo->cinfo, COL_PROTOCOL, "VintageStory");

  // If this packet came from the server port, then this packet is a server
  // packet. Otherwise, it's a client packet. Instead of hard coding the server
  // port to 42420, pinfo.match_uint is used. pinfo.match_uint will be set to
  // 42420 by default. However, if the user added the protocol on another port
  // through the 'Decode As' option, then it will report that port.
  bool is_server = (pinfo->srcport == pinfo->match_uint);

  std::vector<std::string> packet_names;
  gint offset = 0;
  while (tvb_captured_length_remaining(buffer, offset) > 0) {
    gint remaining = tvb_reported_length_remaining(buffer, offset);
    if (remaining < 4) {
      // The packet_length field is 4 bytes. If there are fewer than 4 bytes
      // remaining, that means the packet was truncated. Setting
      // desegment_offset and desegment_nil tells Wireshark to combine the
      // remainder of this packet with the buffer from the next one, then call
      // this dissector again. Specifically desegment_offset is set to `offset`
      // so that this dissector is called again at the start of the length
      // field. `desegment_len` is set to `DESEGMENT_ONE_MORE_SEGMENT` because
      // the exact number of remaining bytes in the packet is currently
      // unknown.
      pinfo->desegment_offset = offset;
      pinfo->desegment_len = DESEGMENT_ONE_MORE_SEGMENT;
      SetInfoColumn(pinfo, packet_names);
      return tvb_captured_length(buffer);
    }

    uint32_t packet_length = tvb_get_ntohl(buffer, offset);
    bool compressed = packet_length & 0x80000000ul;
    packet_length &= 0x7ffffffful;
    if (static_cast<uint32_t>(remaining) < packet_length + 4) {
      // The buffer contains the full length field from the packet, but the
      // payload is truncated. So set `desegment_offset` and `desegment_len` so
      // that Wireshark combines the remaining buffer with the next packet and
      // calls the dissector again. `desegment_offset` points to the beginning
      // of the length field, so that the length field can be decoded again
      // next time this dissector is called.
      pinfo->desegment_offset = offset;
      // Set this to how many more bytes are necessary to decode the packet
      pinfo->desegment_len = 4 + packet_length - remaining;
      SetInfoColumn(pinfo, packet_names);
      return tvb_captured_length(buffer);
    }

    // The "Vintage Story" node on the packet view
    proto_item *protocol_item = proto_tree_add_item(tree, proto_id, buffer,
                                                    offset, 4 + packet_length,
                                                    ENC_NA);
    // This converts the "Vintage Story" node from a leaf node to an interior
    // node.
    proto_tree *protocol_tree = proto_item_add_subtree(protocol_item,
                                                       protocol_ett);
    proto_tree_add_item(protocol_tree, hf_compressed, buffer, offset, 4,
                        ENC_BIG_ENDIAN);
    proto_tree_add_item(protocol_tree, hf_packet_length, buffer, offset, 4,
                        ENC_BIG_ENDIAN);

    tvbuff_t *payload;
    proto_tree *subtree;
    if (!compressed) {
      payload = tvb_new_subset_length(buffer, offset + 4, packet_length);
      subtree = protocol_tree;
    } else {
      // Create a new subtree for the compressed payload.
      proto_item *compressed_item = proto_tree_add_item(protocol_tree,
                                                        hf_compressed_payload,
                                                        buffer, offset + 4,
                                                        packet_length, ENC_NA);
      proto_tree *compressed_tree =
        proto_item_add_subtree(compressed_item, compressed_payload_ett);
      subtree = compressed_tree;

      // Get a raw pointer to the data out of the buffer, so that it can be
      // passed to libzstd.
      const void* compressed_payload = tvb_get_ptr(buffer, offset + 4, packet_length);
      // Determine how big of a buffer to allocate for decompressing the data.
      size_t decomp_size = ZSTD_getFrameContentSize(compressed_payload, packet_length);
      if (decomp_size == ZSTD_CONTENTSIZE_UNKNOWN ||
          decomp_size == ZSTD_CONTENTSIZE_ERROR) {
        // Bad format
        std::cerr << "Unable to decompress" << std::endl;
        return 0;
      }
      // Allocate the buffer for the decompressed data, and do the decompression.
      unsigned char *decomp_buffer =
        static_cast<unsigned char*>(wmem_alloc(pinfo->pool, decomp_size));
      size_t final_decomp_size = ZSTD_decompress(decomp_buffer, decomp_size,
                                                 compressed_payload,
                                                 packet_length);
      if (final_decomp_size > std::numeric_limits<gint>::max()) {
        std::cerr << "Decompressed size, " << final_decomp_size
                  << ", is too large to fit in a Wireshark buffer."
                  << std::endl;
        return 0;
      }

      // Wrap the decompressed data in a tvbuff_t.
      payload = tvb_new_child_real_data(buffer, decomp_buffer,
                                        static_cast<gint>(final_decomp_size),
                                        static_cast<gint>(final_decomp_size));
      add_new_data_source(pinfo, payload, "Decompressed payload");
    }
    packet_names.push_back(DecodePacket(is_server, payload, pinfo, subtree));

    offset += 4 + packet_length;
  }
  SetInfoColumn(pinfo, packet_names);
  return offset;
}

void ProtoRegiser() {
  proto_id = proto_register_protocol(/*name=*/"Vintage Story",
                                     /*short_name=*/"VintageStory",
                                     /*filter_name=*/"vs");
  // This needs to be static so that Wireshark can access the pointer after the
  // function returns.
  static hf_register_info fields[] = {
    {
      &hf_compressed, {
        // Visual Studio complains about mixing designated initializers and
        // non-designated initializers (from HFILL). So avoid using designated
        // initializers.
        /*.name =*/ "compressed",
        /*.abbrev =*/ "vs.compressed",
        /*.type =*/ FT_BOOLEAN,
        /*.display =*/ 32,
        /*.strings =*/ nullptr,
        /*.bitmask =*/ 0x80000000,
        /*.blurb =*/ nullptr,
        HFILL
      }
    },
    {
      &hf_packet_length, {
        /*.name =*/ "packet length",
        /*.abbrev =*/ "vs.length",
        /*.type =*/ FT_UINT32,
        /*.display =*/ BASE_DEC,
        /*.strings =*/ nullptr,
        /*.bitmask =*/ 0x7fffffff,
        /*.blurb =*/ nullptr,
        HFILL
      }
    },
    {
      &hf_compressed_payload, {
        /*.name =*/ "compressed payload",
        /*.abbrev =*/ "vs.compressed_payload",
        /*.type =*/ FT_BYTES,
        /*.display =*/ BASE_NONE,
        /*.strings =*/ nullptr,
        /*.bitmask =*/ 0,
        /*.blurb =*/ nullptr,
        HFILL
      }
    },
    {
      &hf_padding, {
        /*.name =*/ "0 padding",
        /*.abbrev =*/ "vs.zero_padding",
        /*.type =*/ FT_BYTES,
        /*.display =*/ BASE_NONE,
        /*.strings =*/ nullptr,
        /*.bitmask =*/ 0,
        /*.blurb =*/ nullptr,
        HFILL
      }
    },
  };
  proto_register_field_array(proto_id, fields,
                             static_cast<int>(std::size(fields)));

  // This does not need to be static. `proto_register_subtree_array` only uses
  // it as an output array.
  int * const subtree_expansions[] = {
    &protocol_ett,
    &compressed_payload_ett,
  };
  proto_register_subtree_array(subtree_expansions,
                               static_cast<int>(std::size(subtree_expansions)));
}

void ProtoRegHandoff() {
  dissector_handle_t handle = create_dissector_handle(Dissect, proto_id);
  dissector_add_uint("tcp.port", kDefaultServerPort, handle);
}

} // namespace

#if defined(_WIN32) || defined __CYGWIN__
#define EXPORT extern "C" __declspec(dllexport)
#else
#define EXPORT extern "C" __attribute__((visibility("default")))
#endif

// These must be declared extern so that the symbols are exported in the .so
// library, so that Wireshark can find them. Otherwise, the plugin will fail to
// load with a 'has no plugin_version symbol' error.
EXPORT const char plugin_version[] = "1.0.1";
EXPORT const int plugin_want_major = WIRESHARK_VERSION_MAJOR;
EXPORT const int plugin_want_minor = WIRESHARK_VERSION_MINOR;

EXPORT void plugin_register()
{
  // This must be static. Wireshark references it after this function returns.
  static proto_plugin plug;

  plug.register_protoinfo = &ProtoRegiser;
  plug.register_handoff = &ProtoRegHandoff;
  proto_register_plugin(&plug);
}

} // namespace vintage_story
