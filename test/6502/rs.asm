ChannelState:	RSRESET
.io		__RSW	1
.instrument	__RSL	1
.source		__RSL	1
.buffer		__RSL	1
.dma		__RSB	1	; command
.src_bank	__RSB	1
		__RSB	1	; end of job options
		__RSB	1	; DMA_CMD.COPY
		__RSW	1	; BUFFER_SIZE
.src		__RSB	3
.dest		__RSB	3
		__RSB	1	; (DMA_ADDR_MODE.LINEAR<<2)|(DMA_ADDR_MODE.LINEAR)
		__RSW	1	; $0000
		RSEND

		PRINTV ChannelState.dest
		PRINTV ChannelState
