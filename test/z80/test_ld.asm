	SECTION "Test",CODE[0]

Data:	EQU	$1234

	ld		(bc),a  
	ld		(de),a  
	ld		(hl),b
	ld		(hl),l
	ld		(hl),($21)*2
	ld		(ix+1),b
	ld		(ix+2),a
	ld		(ix+3),$42
	ld		(iy+4),b
	ld		(iy+5),l
	ld		(iy+6),$87
	ld		(Data),a  
	ld		(Data),bc 
	ld		(Data),de 
	ld		(Data),hl 
	ld		(Data),ix 
	ld		(Data),iy 
	ld		(Data),sp 

	ld		a,(bc)  
	ld		a,(de)  
	ld		a,(hl)  
	ld		a,(ix+1)
	ld		a,(iy+2)
	ld		a,(Data)  
	ld		a,b
	ld		a,l
	ld		a,i     
	ld		a,$42     
	ld		a,r     

	ld		b,(hl)  
	ld		b,(ix+1)
	ld		b,(iy+2)
	ld		b,a
	ld		b,$87
	ld		bc,(Data)
	ld		bc,Data 

	ld		c,(hl)  
	ld		c,(ix+1)
	ld		c,(iy+2)
	ld		c,l 
	ld		c,$87     

	ld		d,(hl)  
	ld		d,(ix+1)
	ld		d,(iy+2)
	ld		d,l
	ld		d,$87     
	ld		de,(Data) 
	ld		de,Data

	ld		e,(hl)  
	ld		e,(ix+1)
	ld		e,(iy+2)
	ld		e,l
	ld		e,$42

	ld		h,(hl)  
	ld		h,(ix+1)
	ld		h,(iy+2)
	ld		h,l
	ld		h,$87
	ld		hl,(Data) 
	ld		hl,Data

	ld		i,a     
	ld		ix,(Data) 
	ld		ix,Data
	ld		iy,(Data) 
	ld		iy,Data

	ld		l,(hl)  
	ld		l,(ix+1)
	ld		l,(iy+2)
	ld		l,b
	ld		l,$42     

	ld		r,a     
	ld		sp,(Data) 
	ld		sp,hl   
	ld		sp,ix   
	ld		sp,iy   
	ld		sp,Data

	OPT		ms1
	ld		bc,de
	ld		de,hl

	ld		hl,(ix)
	ld		(ix+2),bc
	ld		hl,(ix+64)
	ld		bc,(iy-5)
