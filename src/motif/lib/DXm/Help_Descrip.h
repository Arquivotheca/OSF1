/*
 * *****************************************************************
 * *                                                               *
 * *    Copyright (c) Digital Equipment Corporation, 1991, 1994    *
 * *                                                               *
 * *   All Rights Reserved.  Unpublished rights  reserved  under   *
 * *   the copyright laws of the United States.                    *
 * *                                                               *
 * *   The software contained on this media  is  proprietary  to   *
 * *   and  embodies  the  confidential  technology  of  Digital   *
 * *   Equipment Corporation.  Possession, use,  duplication  or   *
 * *   dissemination of the software and media is authorized only  *
 * *   pursuant to a valid written license from Digital Equipment  *
 * *   Corporation.                                                *
 * *                                                               *
 * *   RESTRICTED RIGHTS LEGEND   Use, duplication, or disclosure  *
 * *   by the U.S. Government is subject to restrictions  as  set  *
 * *   forth in Subparagraph (c)(1)(ii)  of  DFARS  252.227-7013,  *
 * *   or  in  FAR 52.227-19, as applicable.                       *
 * *                                                               *
 * *****************************************************************
 */
/*
 * HISTORY
 */
/*
 *	DESCRIP - VMS Descriptor Definitions
 *	(Based on the VAX-11 Procedure Calling and Condition Handling Standard, Revision 9.0 [7-Dec-81])
 */


/*
 *	General descriptor format - each class of descriptor consists of at least the following fields:
 */
#if defined (VMS) || defined (__VMS)
#include <X11/apienvset.h>
#endif

struct	dsc_descriptor
	{
	unsigned short	dsc_w_length;	/* specific to descriptor class;  typically a 16-bit (unsigned) length */
	unsigned char	dsc_b_dtype;	/* data type code */
	unsigned char	dsc_b_class;	/* descriptor class code */
	char		*dsc_a_pointer;	/* address of first byte of data element */
	};


/*
 *	Scalar or string descriptor:
 */
struct	dsc_descriptor_s
	{
	unsigned short	dsc_w_length;	/* length of data item in bytes,
					     or if dsc_b_dtype is DSC_K_DTYPE_V, bits,
					     or if dsc_b_dtype is DSC_K_DTYPE_P, digits (4 bits each) */
	unsigned char	dsc_b_dtype;	/* data type code */
	unsigned char	dsc_b_class;	/* descriptor class code = DSC_K_CLASS_S */
	char		*dsc_a_pointer;	/* address of first byte of data storage */
	};


/*
 *	Dynamic string descriptor:
 */
struct	dsc_descriptor_d
	{
	unsigned short	dsc_w_length;	/* length of data item in bytes,
					     or if dsc_b_dtype is DSC_K_DTYPE_V, bits,
					     or if dsc_b_dtype is DSC_K_DTYPE_P, digits (4 bits each) */
	unsigned char	dsc_b_dtype;	/* data type code */
	unsigned char	dsc_b_class;	/* descriptor class code = DSC_K_CLASS_D */
	char		*dsc_a_pointer;	/* address of first byte of data storage */
	};


/*
 *	Array descriptor:
 */
struct	dsc_descriptor_a
	{
	unsigned short	dsc_w_length;	/* length of an array element in bytes,
					     or if dsc_b_dtype is DSC_K_DTYPE_V, bits,
					     or if dsc_b_dtype is DSC_K_DTYPE_P, digits (4 bits each) */
	unsigned char	dsc_b_dtype;	/* data type code */
	unsigned char	dsc_b_class;	/* descriptor class code = DSC_K_CLASS_A */
	char		*dsc_a_pointer;	/* address of first actual byte of data storage */
	char		dsc_b_scale;	/* scale multiplier to convert from internal to external form */
	unsigned char	dsc_b_digits;	/* number of decimal digits in internal representation */
	struct	{
		unsigned		 : 4;	/* reserved, must be zero */
		unsigned dsc_v_fl_redim	 : 1;	/* if set, indicates the array can be redimensioned */
		unsigned dsc_v_fl_column : 1;	/* if set, indicates column-major order (FORTRAN) */
		unsigned dsc_v_fl_coeff  : 1;	/* if set, indicates the multipliers block is present */
		unsigned dsc_v_fl_bounds : 1;	/* if set, indicates the bounds block is present */
		}	dsc_b_aflags;
	unsigned char	dsc_b_dimct;	/* number of dimensions */
	unsigned long	dsc_l_arsize;	/* total size of array in bytes,
					     or if dsc_b_dtype is DSC_K_DTYPE_P, digits (4 bits each) */
	/*
	 * One or two optional blocks of information may follow contiguously at this point;
	 * the first block contains information about the dimension multipliers (if present,
	 * dsc_b_aflags.dsc_v_fl_coeff is set), the second block contains information about
	 * the dimension bounds (if present, dsc_b_aflags.dsc_v_fl_bounds is set).  If the
	 * bounds information is present, the multipliers information must also be present.
	 *
	 * The multipliers block has the following format:
	 *	char	*dsc_a_a0;		Address of the element whose subscripts are all zero
	 *	long	dsc_l_m [DIMCT];	Addressing coefficients (multipliers)
	 *
	 * The bounds block has the following format:
	 *	struct 	{
	 *		long	dsc_l_l;	Lower bound
	 *		long	dsc_l_u;	Upper bound
	 *		} dsc_bounds [DIMCT];
	 *
	 * (DIMCT represents the value contained in dsc_b_dimct.)
	 */
	};


/*
 *	Procedure descriptor:
 */
struct	dsc_descriptor_p
	{
	unsigned short	dsc_w_length;	/* length associated with the function value */
	unsigned char	dsc_b_dtype;	/* function value data type code */
	unsigned char	dsc_b_class;	/* descriptor class code = DSC_K_CLASS_P */
	char		*dsc_a_pointer;	/* address of function entry mask */
	};


/*
 *	Decimal scalar string descriptor:
 */
struct	dsc_descriptor_sd
	{
	unsigned short	dsc_w_length;	/* length of data item in bytes,
					     or if dsc_b_dtype is DSC_K_DTYPE_V, bits,
					     or if dsc_b_dtype is DSC_K_DTYPE_P, digits (4 bits each) */
	unsigned char	dsc_b_dtype;	/* data type code */
	unsigned char	dsc_b_class;	/* descriptor class code = DSC_K_CLASS_SD */
	char		*dsc_a_pointer;	/* address of first byte of data storage */
	char		dsc_b_scale;	/* scale multiplier to convert from internal to external form */
	unsigned char	dsc_b_digits;	/* number of decimal digits in internal representation */
	unsigned	: 16;		/* reserved for future use, must be zero */
	};


/*
 *	Noncontiguous array descriptor:
 */
struct	dsc_descriptor_nca
	{
	unsigned short	dsc_w_length;	/* length of an array element in bytes,
					     or if dsc_b_dtype is DSC_K_DTYPE_V, bits,
					     or if dsc_b_dtype is DSC_K_DTYPE_P, digits (4 bits each) */
	unsigned char	dsc_b_dtype;	/* data type code */
	unsigned char	dsc_b_class;	/* descriptor class code = DSC_K_CLASS_NCA */
	char		*dsc_a_pointer;	/* address of first actual byte of data storage */
	char		dsc_b_scale;	/* scale multiplier to convert from internal to external form */
	unsigned char	dsc_b_digits;	/* number of decimal digits in internal representation */
	struct	{
		unsigned		 : 4;	/* reserved for future standardization, must be zero */
		unsigned dsc_v_fl_redim	 : 1;	/* must be zero */
		unsigned 		 : 3;	/* reserved for future standardization, must be zero */
		}	dsc_b_aflags;
	unsigned char	dsc_b_dimct;	/* number of dimensions */
	unsigned long	dsc_l_arsize;	/* total size of array in bytes,
					     or if dsc_b_dtype is DSC_K_DTYPE_P, digits (4 bits each) */
	/*
	 * Two blocks of information must follow contiguously at this point;  the first block
	 * contains information about the difference between the addresses of two adjacent
	 * elements in each dimension (the stride).  The second block contains information
	 * about the dimension bounds.
	 *
	 * The strides block has the following format:
	 *	char		*dsc_a_a0;		Address of the element whose subscripts are all zero
	 *	unsigned long	dsc_l_s [DIMCT];	Strides
	 *
	 * The bounds block has the following format:
	 *	struct 	{
	 *		long	dsc_l_l;		Lower bound
	 *		long	dsc_l_u;		Upper bound
	 *		} dsc_bounds [DIMCT];
	 *
	 * (DIMCT represents the value contained in dsc_b_dimct.)
	 */
	};


/*
 *	The varying string descriptor and varying string array descriptor are used with strings
 *	of the following form:
 *
 *		struct	{
 *			unsigned short	CURLEN;		The current length of BODY in bytes
 *			char	BODY [MAXSTRLEN];	A fixed-length area containing the string
 *			};
 *
 *	where MAXSTRLEN is the value contained in the dsc_w_maxstrlen field in the descriptor.
 */

/*
 *	Varying string descriptor:
 */
struct	dsc_descriptor_vs
	{
	unsigned short	dsc_w_maxstrlen; /* maximum length of the BODY field of the varying string in bytes */
	unsigned char	dsc_b_dtype;	/* data type code = DSC_K_DTYPE_VT */
	unsigned char	dsc_b_class;	/* descriptor class code = DSC_K_CLASS_VS */
	char		*dsc_a_pointer;	/* address of the CURLEN field of the varying string */
	};


/*
 *	Varying string array descriptor:
 */
struct	dsc_descriptor_vsa
	{
	unsigned short	dsc_w_maxstrlen; /* maximum length of the BODY field of an array element in bytes */
	unsigned char	dsc_b_dtype;	/* data type code = DSC_K_DTYPE_VT */
	unsigned char	dsc_b_class;	/* descriptor class code = DSC_K_CLASS_VSA */
	char		*dsc_a_pointer;	/* address of first actual byte of data storage */
	char		dsc_b_scale;	/* reserved for future standardization, must be zero */
	unsigned char	dsc_b_digits;	/* reserved for future standardization, must be zero */
	struct	{
		unsigned		 : 4;	/* reserved for future standardization, must be zero */
		unsigned dsc_v_fl_redim	 : 1;	/* must be zero */
		unsigned 		 : 3;	/* reserved for future standardization, must be zero */
		}	dsc_b_aflags;
	unsigned char	dsc_b_dimct;	/* number of dimensions */
	unsigned long	dsc_l_arsize;	/* total size of array in bytes */
	/*
	 * Two blocks of information must follow contiguously at this point;  the first block
	 * contains information about the difference between the addresses of two adjacent
	 * elements in each dimension (the stride).  The second block contains information
	 * about the dimension bounds.
	 *
	 * The strides block has the following format:
	 *	char		*dsc_a_a0;		Address of the element whose subscripts are all zero
	 *	unsigned long	dsc_l_s [DIMCT];	Strides
	 *
	 * The bounds block has the following format:
	 *	struct 	{
	 *		long	dsc_l_l;		Lower bound
	 *		long	dsc_l_u;		Upper bound
	 *		} dsc_bounds [DIMCT];
	 *
	 * (DIMCT represents the value contained in dsc_b_dimct.)
	 */
	};


/*
 *	Unaligned bit string descriptor:
 */
struct	dsc_descriptor_ubs
	{
	unsigned short	dsc_w_length;	/* length of data item in bits */
	unsigned char	dsc_b_dtype;	/* data type code = DSC_K_DTYPE_VU */
	unsigned char	dsc_b_class;	/* descriptor class code = DSC_K_CLASS_UBS */
	char		*dsc_a_base;	/* address to which dsc_l_pos is relative */
	long		dsc_l_pos;	/* bit position relative to dsc_a_base of first bit in string */
	};


/*
 *	Unaligned bit array descriptor:
 */
struct	dsc_descriptor_uba
	{
	unsigned short	dsc_w_length;	/* length of data item in bits */
	unsigned char	dsc_b_dtype;	/* data type code = DSC_K_DTYPE_VU */
	unsigned char	dsc_b_class;	/* descriptor class code = DSC_K_CLASS_UBA */
	char		*dsc_a_base;	/* address to which effective bit offset is relative */
	char		dsc_b_scale;	/* reserved for future standardization, must be zero */
	unsigned char	dsc_b_digits;	/* reserved for future standardization, must be zero */
	struct	{
		unsigned		 : 4;	/* reserved for future standardization, must be zero */
		unsigned dsc_v_fl_redim	 : 1;	/* must be zero */
		unsigned 		 : 3;	/* reserved for future standardization, must be zero */
		}	dsc_b_aflags;
	unsigned char	dsc_b_dimct;	/* number of dimensions */
	unsigned long	dsc_l_arsize;	/* total size of array in bits */
	/*
	 * Three blocks of information must follow contiguously at this point;  the first block
	 * contains information about the difference between the bit addresses of two adjacent
	 * elements in each dimension (the stride).  The second block contains information
	 * about the dimension bounds.  The third block is the relative bit position with
	 * respect to dsc_a_base of the first actual bit of the array.
	 *
	 * The strides block has the following format:
	 *	long		dsc_l_v0;		Bit offset of the element whose subscripts are all zero,
	 *						with respect to dsc_a_base
	 *	unsigned long	dsc_l_s [DIMCT];	Strides
	 *
	 * The bounds block has the following format:
	 *	struct 	{
	 *		long	dsc_l_l;		Lower bound
	 *		long	dsc_l_u;		Upper bound
	 *		} dsc_bounds [DIMCT];
	 *
	 * The last block has the following format:
	 *	long	dsc_l_pos;
	 *
	 * (DIMCT represents the value contained in dsc_b_dimct.)
	 */
	};


/*
 *	Codes for dsc_b_dtype:
 */

/*
 *	Atomic data types:
 */
#define DSC_K_DTYPE_Z	0		/* unspecified */
#define DSC_K_DTYPE_BU	2		/* byte logical;  8-bit unsigned quantity */
#define DSC_K_DTYPE_WU	3		/* word logical;  16-bit unsigned quantity */
#define DSC_K_DTYPE_LU	4		/* longword logical;  32-bit unsigned quantity */
#define DSC_K_DTYPE_QU	5		/* quadword logical;  64-bit unsigned quantity */
#define DSC_K_DTYPE_OU	25		/* octaword logical;  128-bit unsigned quantity */
#define DSC_K_DTYPE_B	6		/* byte integer;  8-bit signed 2's-complement integer */
#define DSC_K_DTYPE_W	7		/* word integer;  16-bit signed 2's-complement integer */
#define DSC_K_DTYPE_L	8		/* longword integer;  32-bit signed 2's-complement integer */
#define DSC_K_DTYPE_Q	9		/* quadword integer;  64-bit signed 2's-complement integer */
#define DSC_K_DTYPE_O	26		/* octaword integer;  128-bit signed 2's-complement integer */
#define DSC_K_DTYPE_F	10		/* F_floating;  32-bit single-precision floating point */
#define DSC_K_DTYPE_D	11		/* D_floating;  64-bit double-precision floating point */
#define DSC_K_DTYPE_G	27		/* G_floating;  64-bit double-precision floating point */
#define DSC_K_DTYPE_H	28		/* H_floating;  128-bit quadruple-precision floating point */
#define DSC_K_DTYPE_FC	12		/* F_floating complex */
#define DSC_K_DTYPE_DC	13		/* D_floating complex */
#define DSC_K_DTYPE_GC	29		/* G_floating complex */
#define DSC_K_DTYPE_HC	30		/* H_floating complex */
#define DSC_K_DTYPE_CIT	31		/* COBOL Intermediate Temporary */
/*
 *	String data types:
 */
#define DSC_K_DTYPE_T	14		/* character-coded text;  a single character or a string */
#define DSC_K_DTYPE_VT	37		/* varying character-coded text;  16-bit count, followed by a string */
#define DSC_K_DTYPE_NU	15		/* numeric string, unsigned */
#define DSC_K_DTYPE_NL	16		/* numeric string, left separate sign */
#define DSC_K_DTYPE_NLO	17		/* numeric string, left overpunched sign */
#define DSC_K_DTYPE_NR	18		/* numeric string, right separate sign */
#define DSC_K_DTYPE_NRO	19		/* numeric string, right overpunched sign */
#define DSC_K_DTYPE_NZ	20		/* numeric string, zoned sign */
#define DSC_K_DTYPE_P	21		/* packed decimal string */
#define DSC_K_DTYPE_V	1		/* bit;  aligned bit string */
#define DSC_K_DTYPE_VU	34		/* bit unaligned;  arbitrary bit string */
/*
 *	Miscellaneous data types:
 */
#define DSC_K_DTYPE_ZI	22		/* sequence of instructions */
#define DSC_K_DTYPE_ZEM	23		/* procedure entry mask */
#define DSC_K_DTYPE_DSC	24		/* descriptor */
#define DSC_K_DTYPE_BPV	32		/* bound procedure value */
#define DSC_K_DTYPE_BLV	33		/* bound label value */
#define DSC_K_DTYPE_ADT	35		/* absolute date and time */
/*
 *	Reserved data type codes:
 *	codes 38-191 are reserved to DIGITAL;
 *	codes 160-191 are reserved to DIGITAL facilities for facility-specific purposes;
 *	codes 192-255 are reserved for DIGITAL's Computer Special Systems Group
 *	and for customers for their own use.
 */


/*
 *	Codes for dsc_b_class:
 */
#define DSC_K_CLASS_S	1		/* scalar or string descriptor */
#define DSC_K_CLASS_D	2		/* dynamic string descriptor */
/*	DSC_K_CLASS_V	3		** variable buffer descriptor;  reserved for use by DIGITAL */
#define DSC_K_CLASS_A	4		/* array descriptor */
#define DSC_K_CLASS_P	5		/* procedure descriptor */
/*	DSC_K_CLASS_PI	6		** procedure incarnation descriptor;  obsolete */
/*	DSC_K_CLASS_J	7		** label descriptor;  reserved for use by the VAX-11 Debugger */
/*	DSC_K_CLASS_JI	8		** label incarnation descriptor;  obsolete */
#define DSC_K_CLASS_SD	9		/* decimal scalar string descriptor */
#define DSC_K_CLASS_NCA	10		/* noncontiguous array descriptor */
#define DSC_K_CLASS_VS	11		/* varying string descriptor */
#define DSC_K_CLASS_VSA	12		/* varying string array descriptor */
#define DSC_K_CLASS_UBS	13		/* unaligned bit string descriptor */
#define DSC_K_CLASS_UBA	14		/* unaligned bit array descriptor */
/*
 *	Reserved descriptor class codes:
 *	codes 15-191 are reserved to DIGITAL;
 *	codes 160-191 are reserved to DIGITAL facilities for facility-specific purposes;
 *	codes 192-255 are reserved for DIGITAL's Computer Special Systems Group
 *	and for customers for their own use.
 */


/*
 *	A simple macro to construct a string descriptor:
 */
#define _DESCRIPTOR(name,string)	struct dsc_descriptor_s name = { sizeof(string)-1, DSC_K_DTYPE_T, DSC_K_CLASS_S, string }

#if defined(VMS) || defined (__VMS)
#include <X11/apienvrst.h>
#endif
