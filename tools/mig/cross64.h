/*
  MACHINA: Assume 64-bit for all architectures.
*/

#define	word_size			8
#define	word_size_in_bits		64
#define	word_size_name			MCN_MSGTYPE_INT64
#define	word_size_name_string		"MCN_MSGTYPE_INT64"

#define	sizeof_pointer			8
#define	sizeof_mach_msg_header_t	sizeof(mach_msg_header_t)
#define	sizeof_mach_msg_type_long_t	sizeof(mcn_msgtype_long_t)
#define	sizeof_mach_msg_type_t		sizeof(mcn_msgtype_t)

