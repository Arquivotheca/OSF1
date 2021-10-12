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
/* $XConsortium: PEXOCs.h,v 5.2 91/02/16 09:47:07 rws Exp $ */

/***********************************************************
Copyright 1989, 1990, 1991 by Sun Microsystems, Inc. and the X Consortium.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Sun Microsystems,
the X Consortium, and MIT not be used in advertising or publicity 
pertaining to distribution of the software without specific, written 
prior permission.  

SUN MICROSYSTEMS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, 
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT 
SHALL SUN MICROSYSTEMS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL 
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/



#ifndef	OCDEFS_H
#define	OCDEFS_H

#define	OC_MARKER_TYPE			 	 	1L
#define	OC_MARKER_SCALE			 	 	2L
#define	OC_MARKER_COLOR_INDEX		 	 	3L
#define	OC_MARKER_COLOR			 	 	4L
#define	OC_MARKER_BUNDLEINDEX		 	 	5L
#define	OC_TEXT_FONT			 	 	6L
#define	OC_TEXT_PRECISION		 	 	7L
#define	OC_CHARACTER_EXPANSION		 	 	8L
#define	OC_CHARACTER_SPACING		 	 	9L
#define	OC_TEXT_COLOR_INDEX				10L
#define	OC_TEXT_COLOR					11L
#define	OC_CHARACTER_HEIGHT				12L
#define	OC_CHARACTER_UP_VECTOR				13L
#define	OC_TEXT_PATH					14L
#define	OC_TEXT_ALIGNMENT				15L
#define	OC_ANNOTATION_TEXT_HEIGHT			16L
#define	OC_ANNOTATION_TEXT_UP_VECTOR			17L
#define	OC_ANNOTATION_TEXT_PATH				18L
#define	OC_ANNOTATION_TEXT_ALIGNMENT			19L
#define	OC_ANNOTATION_TEXT_STYLE			20L
#define	OC_TEXT_BUNDLE_INDEX				21L
#define	OC_LINE_TYPE					22L
#define	OC_LINE_WIDTH					23L
#define	OC_LINE_COLOR_INDEX				24L
#define	OC_LINE_COLOR					25L
#define	OC_CURVE_APPROXIMATION_METHOD			26L
#define	OC_POLYLINE_INTERPOLATION_METHOD		27L
#define	OC_LINE_BUNDLE_INDEX				28L
#define	OC_SURFACE_INTERIOR_STYLE			29L
#define	OC_SURFACE_INTERIOR_STYLE_INDEX			30L
#define	OC_SURFACE_COLOR_INDEX				31L
#define	OC_SURFACE_COLOR				32L
#define	OC_SURFACE_REFLECTION_ATTRIBUTES		33L
#define	OC_SURFACE_REFLECTION_MODEL			34L
#define	OC_SURFACE_INTERPOLATION_METHOD			35L
#define	OC_BACKFACE_SURFACE_INTERIOR_STYLE		36L
#define	OC_BACKFACE_SURFACE_INTERIOR_STYLE_INDEX	37L
#define	OC_BACKFACE_SURFACE_COLOR_INDEX			38L
#define	OC_BACKFACE_SURFACE_COLOR			39L
#define	OC_BACKFACE_SURFACE_REFLECTION_ATTRIBUTES	40L
#define	OC_BACKFACE_SURFACE_REFLECTION_MODEL		41L
#define	OC_BACKFACE_SURFACE_INTERPOLATION_METHOD	42L
#define	OC_SURFACE_APPROXIMATION_METHOD			43L
#define	OC_TRIM_CURVE_APPROXIMATION_METHOD		44L
#define	OC_FACET_CULLING_MODE				45L
#define	OC_FACET_DISTINGUISH_FLAG			46L
#define	OC_NORMAL_REORIENTATION_MODE			47L
#define	OC_PATTERN_SIZE					48L
#define	OC_PATTERN_REFERENCE_POINT			49L
#define	OC_PATTERN_ATTRIBUTES				50L
#define	OC_INTERIOR_BUNDLE_INDEX			51L
#define	OC_SURFACE_EDGE_FLAG				52L
#define	OC_SURFACE_EDGE_TYPE				53L
#define	OC_SURFACE_EDGE_WIDTH				54L
#define	OC_SURFACE_EDGE_COLOR_INDEX			55L
#define	OC_SURFACE_EDGE_COLOR				56L
#define	OC_EDGE_BUNDLE_INDEX				57L
#define	OC_SET_INDIVIDUAL_ASF				58L
#define	OC_LOCAL_TRANSFORM_3D				59L
#define	OC_LOCAL_TRANSFORM_2D				60L
#define	OC_GLOBAL_TRANSFORM_3D				61L
#define	OC_GLOBAL_TRANSFORM_2D				62L
#define	OC_MODEL_CLIP					63L
#define	OC_SET_MODEL_CLIP_VOLUME_3D			64L
#define	OC_SET_MODEL_CLIP_VOLUME_2D			65L
#define	OC_RESTORE_MODEL_CLIP_VOLUME			66L
#define	OC_VIEW_INDEX					67L
#define	OC_LIGHTSOURCE_STATE				68L
#define	OC_DEPTH_CUE_INDEX				69L
#define	OC_PICK_ID					70L
#define	OC_HLHSR_IDENTIFIER				71L
#define	OC_ADD_NAMES_TO_NAME_SET			72L
#define	OC_REMOVE_NAMES_TO_NAME_SET			73L
#define	OC_EXECUTE_STRUCTURE				74L
#define	OC_LABEL					75L
#define	OC_APPLICATION_DATA				76L
#define	OC_GSE						77L
#define	OC_MARKER_3D					78L
#define	OC_MARKER_2D					79L
#define	OC_TEXT_3D					80L
#define	OC_TEXT_2D					81L
#define	OC_ANNOTATION_TEXT_3D				82L
#define	OC_ANNOTATION_TEXT_2D				83L
#define	OC_POLYLINE_3D					84L
#define	OC_POLYLINE_2D					85L
#define	OC_POLYLINE_SET_3D_WITH_DATA			86L
#define	OC_PARAMETRIC_CURVE				87L
#define	OC_NURB						88L
#define	OC_FILL_AREA_3D					89L
#define	OC_FILL_AREA_2D					90L
#define	OC_FILL_AREA_3D_WITH_DATA			91L
#define	OC_FILL_AREA_SET_3D				92L
#define	OC_FILL_AREA_SET_2D				93L
#define	OC_FILL_AREA_SET_3D_WITH_DATA			94L
#define	OC_TRIANGLE_STRIP				95L
#define	OC_QUADRILATERAL_MESH				96L
#define	OC_INDEXED_POLYGON				97L
#define	OC_PARAMETRIC_POLYNOMIAL_SURFACE		98L
#define	OC_NURB_SURFACE					99L
#define	OC_CELL_ARRAY_3D				100L
#define	OC_CELL_ARRAY_2D				101L
#define	OC_EXTENDED_CELL_ARRAY_3D			102L
#define	OC_GDP_3D					103L
#define	OC_GDP_2D					104L
#define	MAXOUTCMDS					105

#endif	/* OCDEFS_H */

