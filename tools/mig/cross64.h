/*
  MACHINA: Assume 64-bit for all architectures.
*/

#define	word_size			(Is32Bit ? 4 : 8)
#define	word_size_in_bits		(Is32Bit ? 32 : 64)
#define	word_size_name			(Is32Bit ? MCN_MSGTYPE_INT32 : MCN_MSGTYPE_INT64)
#define	word_size_name_string		(Is32Bit ? "MCN_MSGTYPE_INT32" : "MCN_MSGTYPE_INT64")
#define	sizeof_pointer			(Is32Bit ? 4 : 8)

#define sizeof_mcn_msgsend_t		sizeof(mcn_msgsend_t)
#define sizeof_mcn_msgrecv_t		sizeof(mcn_msgrecv_t)
#define	sizeof_mcn_msgtype_long_t	sizeof(mcn_msgtype_long_t)
#define	sizeof_mcn_msgtype_t		sizeof(mcn_msgtype_t)
