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
** Generated by ODL Version X0.1 on Tue Jul 14 18:32:20 1992
*/

/*
** COPYRIGHT (c) 1989 BY
** DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASSACHUSETTS.
** ALL RIGHTS RESERVED.
**
** THIS SOFTWARE IS FURNISHED UNDER A LICENSE AND MAY BE USED AND COPIED
** ONLY  IN  ACCORDANCE  OF  THE  TERMS  OF  SUCH  LICENSE  AND WITH THE
** INCLUSION OF THE ABOVE COPYRIGHT NOTICE. THIS SOFTWARE OR  ANY  OTHER
** COPIES THEREOF MAY NOT BE PROVIDED OR OTHERWISE MADE AVAILABLE TO ANY
** OTHER PERSON.  NO TITLE TO AND  OWNERSHIP OF THE  SOFTWARE IS  HEREBY
** TRANSFERRED.
**
** THE INFORMATION IN THIS SOFTWARE IS  SUBJECT TO CHANGE WITHOUT NOTICE
** AND  SHOULD  NOT  BE  CONSTRUED  AS A COMMITMENT BY DIGITAL EQUIPMENT
** CORPORATION.
**
** DIGITAL ASSUMES NO RESPONSIBILITY FOR THE USE  OR  RELIABILITY OF ITS
** SOFTWARE ON EQUIPMENT WHICH IS NOT SUPPLIED BY DIGITAL.
*/

/*
** Private Type Definitions for the ObjectId Object
*/

/*
** Abstract Type Definitions for subtypes of Object
*/

typedef _Object _List, _PtrTo _ListPtr;
typedef _Object _Set, _PtrTo _SetPtr;
typedef _Object _Property, _PtrTo _PropertyPtr;
typedef _Object _ObjectDesc, _PtrTo _ObjectDescPtr;
typedef _Object _Ui, _PtrTo _UiPtr;
typedef _Object _DXmUi, _PtrTo _DXmUiPtr;
typedef _Object _DXmEnvState, _PtrTo _DXmEnvStatePtr;
typedef _Object _Persistent, _PtrTo _PersistentPtr;
typedef _Object _Surrogate, _PtrTo _SurrogatePtr;
typedef _Object _Link, _PtrTo _LinkPtr;
typedef _Object _Linknet, _PtrTo _LinknetPtr;
typedef _Object _CompLinknet, _PtrTo _CompLinknetPtr;
typedef _Object _Step, _PtrTo _StepPtr;
typedef _Object _Path, _PtrTo _PathPtr;
typedef _Object _CompPath, _PtrTo _CompPathPtr;
typedef _Object _Linkbase, _PtrTo _LinkbasePtr;

/*
** ObjectId Instance Structure Definition
*/

typedef struct __ObjectId {
        _Type type;
        _Domain object_domain;
        _Integer object_id;
        _Integer container_id;
        _String linkbase_id;
    } _ObjectIdInstance, _PtrTo _ObjectId, _PtrTo _PtrTo _ObjectIdPtr;

/*
** ObjectId Property Name Table Index Definitions
*/

#define _ObjectDomainIndex 0
#define _ObjectIdentifierIndex 1
#define _ContainerIdentifierIndex 2
#define _LinkbaseIdentifierIndex 3

/*
** ObjectId Property Name Table Definition
*/

#define _ObjectIdPropertyNameTable \
    _PropertyNameTableEntry _Constant ObjectIdPropertyNameTable[] = { \
        {_P_ObjectDomain, _ObjectDomainIndex, _True}, \
        {_P_ObjectIdentifier, _ObjectIdentifierIndex, _True}, \
        {_P_ContainerIdentifier, _ContainerIdentifierIndex, _True}, \
        {_P_LinkbaseIdentifier, _LinkbaseIdentifierIndex, _True}, \
        {(_String) 0, 0, _False} \
    }

/*
** ObjectId Property Accessor Definitions
*/

#define _ObjectDomain_of(Obj) ((_ObjectId) _Reference(Obj))->object_domain
#define _ObjectIdentifier_of(Obj) ((_ObjectId) _Reference(Obj))->object_id
#define _ContainerIdentifier_of(Obj) ((_ObjectId) _Reference(Obj))->container_id
#define _LinkbaseIdentifier_of(Obj) ((_ObjectId) _Reference(Obj))->linkbase_id

/*
** Include Generic Operation Support
*/

#include "lwk_operation_prototypes.h"

/*
** ObjectId Method Declarations
*/

_DeclareFunction(void LwkOpObjIllOp, (_in _Object object));
_DeclareFunction(_ObjectId LwkOpObjCopy, (_in _ObjectId object, _in _Boolean aggreate));
_DeclareFunction(_ObjectId LwkOpObjCreate, (_in _Type type));
_DeclareFunction(void LwkOpOidDecode, (_inout _ObjectId oid,
    _in _DDIShandle handle, _in _Boolean keys_only,
    _inout _SetPtr properties));
_DeclareFunction(void LwkOpOidEncode, (_in _ObjectId oid,
    _in _Boolean aggregate, _in _DDIShandle handle));
_DeclareFunction(_Set LwkOpOidEnumProps, (_in _ObjectId object));
_DeclareFunction(_Integer LwkOpOidExport, (_in _ObjectId oid,
    _in _Boolean aggregate, _out _VaryingStringPtr encoding));
_DeclareFunction(void LwkOpObjExpunge, (_inout _ObjectId object));
_DeclareFunction(void LwkOpOidFree, (_inout _ObjectId object));
_DeclareFunction(_Domain LwkOpObjGetDomain, (_in _ObjectId object));
_DeclareFunction(void LwkOpOidGetValue, (_in _ObjectId object,
    _in _String property_name, _in _Domain domain,
    _in _AnyPtr value));
_DeclareFunction(_Domain LwkOpOidGetValueDomain, (_in _ObjectId object,
    _in _String property_name));
_DeclareFunction(_List LwkOpOidGetValueList, (_in _ObjectId object,
    _in _String property_name));
_DeclareFunction(_ObjectId LwkOpOidImport, (_in _Type type,
    _in _VaryingString encoding));
_DeclareFunction(void LwkOpOidInitialize, (_inout _ObjectId object,
    _in _ObjectId proto_object, _Boolean aggregate));
_DeclareFunction(_Boolean LwkOpOidIsMultiValued, (_in _ObjectId object,
    _in _String property_name));
_DeclareFunction(_Boolean LwkOpObjIsType, (_in _ObjectId object,
    _in _Type type));
_DeclareFunction(_Persistent LwkOpOidRetrieve, (_in _ObjectId id));
_DeclareFunction(void LwkOpOidSetValue, (_inout _ObjectId object,
    _in _String property_name, _in _Domain domain,
    _in _AnyPtr value, _in _SetFlag flag));
_DeclareFunction(void LwkOpOidSetValueList, (_inout _ObjectId object,
    _in _String property_name, _in _List_of(_Domain) value_list,
    _in _SetFlag flag));

/*
** ObjectId Type Instance Definition
*/

#define _ObjectIdTypeInstance \
    _TypeInstance _Constant LWK__ObjectIdTypeInstance = { \
        &LWK__ObjectTypeInstance, \
        sizeof(_ObjectIdInstance), \
        { \
        (_Method) LwkOpObjIllOp, \
        (_Method) LwkOpObjIllOp, \
        (_Method) LwkOpObjIllOp, \
        (_Method) LwkOpObjIllOp, \
        (_Method) LwkOpObjIllOp, \
        (_Method) LwkOpObjIllOp, \
        (_Method) LwkOpObjIllOp, \
        (_Method) LwkOpObjCopy, \
        (_Method) LwkOpObjCreate, \
        (_Method) LwkOpObjIllOp, \
        (_Method) LwkOpObjIllOp, \
        (_Method) LwkOpObjIllOp, \
        (_Method) LwkOpOidDecode, \
        (_Method) LwkOpObjIllOp, \
        (_Method) LwkOpObjIllOp, \
        (_Method) LwkOpObjIllOp, \
        (_Method) LwkOpOidEncode, \
        (_Method) LwkOpOidEnumProps, \
        (_Method) LwkOpOidExport, \
        (_Method) LwkOpObjExpunge, \
        (_Method) LwkOpOidFree, \
        (_Method) LwkOpObjGetDomain, \
        (_Method) LwkOpObjIllOp, \
        (_Method) LwkOpObjIllOp, \
        (_Method) LwkOpOidGetValue, \
        (_Method) LwkOpOidGetValueDomain, \
        (_Method) LwkOpOidGetValueList, \
        (_Method) LwkOpObjIllOp, \
        (_Method) LwkOpOidImport, \
        (_Method) LwkOpObjIllOp, \
        (_Method) LwkOpOidInitialize, \
        (_Method) LwkOpOidIsMultiValued, \
        (_Method) LwkOpObjIllOp, \
        (_Method) LwkOpObjIsType, \
        (_Method) LwkOpObjIllOp, \
        (_Method) LwkOpObjIllOp, \
        (_Method) LwkOpObjIllOp, \
        (_Method) LwkOpObjIllOp, \
        (_Method) LwkOpObjIllOp, \
        (_Method) LwkOpOidRetrieve, \
        (_Method) LwkOpObjIllOp, \
        (_Method) LwkOpObjIllOp, \
        (_Method) LwkOpObjIllOp, \
        (_Method) LwkOpObjIllOp, \
        (_Method) LwkOpObjIllOp, \
        (_Method) LwkOpOidSetValue, \
        (_Method) LwkOpOidSetValueList, \
        (_Method) LwkOpObjIllOp, \
        (_Method) LwkOpObjIllOp, \
        (_Method) LwkOpObjIllOp, \
        (_Method) LwkOpObjIllOp, \
        (_Method) LwkOpObjIllOp, \
        (_Method) LwkOpObjIllOp \
        } \
    }

#define _ObjectIdType \
    _Type _Constant LWK__ObjectIdType = &LWK__ObjectIdTypeInstance
