#if COMMENT

Format of the strings file:
	ASCIZ	signature "StringStore"
	LONG	type "Str"
	UINT64	unique file ID (same as in index file)
	N times:
	   BYTE	 user flags
	   ASCIZ string		(aligned to multiple of granularity)
   (if necessary, NULs are used to pad alignment-enforced gaps between strings)

Format of the index file:
	ASCIZ	signature "StringStore"
	LONG	type "Idx"
	BYTE	file format version (1)
   	BYTE	granularity shift count (0=1 byte, 1=2 bytes, 2=4 bytes, etc)
	BYTE	character encoding
	BYTE	reserved (0)
	UINT64	unique file ID (same as in strings file)
	LONG	last-modified timestamp
	LONG	number of strings in this index
	LONG	offset in strings file past last indexed string, i.e. offset of
		next string to be added	(in multiples of granularity)
   	24 BYTEs reserved (0)
	N LONGs offsets of strings, sorted lexically
		(in multiples of granularity, i.e. shifted right by
		 "granularity shift")

#endif /* COMMENT */
