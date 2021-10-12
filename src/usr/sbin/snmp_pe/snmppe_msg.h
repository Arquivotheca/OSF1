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
 * @(#)$RCSfile: snmppe_msg.h,v $ $Revision: 1.1.2.2 $ (DEC) $Date: 1993/05/20 18:33:24 $
 */
/*
**  Copyright (c) Digital Equipment Corporation, 1991, 1992
**  All Rights Reserved.  Unpublished rights reserved
**  under the copyright laws of the United States.
**  
**  The software contained on this media is proprietary
**  to and embodies the confidential technology of 
**  Digital Equipment Corporation.  Possession, use,
**  duplication or dissemination of the software and
**  media is authorized only pursuant to a valid written
**  license from Digital Equipment Corporation.
**
**  RESTRICTED RIGHTS LEGEND   Use, duplication, or 
**  disclosure by the U.S. Government is subject to
**  restrictions as set forth in Subparagraph (c)(1)(ii)
**  of DFARS 252.227-7013, or in FAR 52.227-19, as
**  applicable.
**
*/

/*
  October 1991 - Converted for use with the SNMP Protocol Engine of the
                 Common Agent.  The name of the file is changed to avoid
                 conflict with the MCC file "mcc_msg.h".
*/
 
/*** MODULE $MCC_DEF ***/
/* #define MCC$_FACILITY 806  -- NOT USED; AND NOT ANSI-C COMPLIANT */
#define MCC_S_NORMAL 52854793
#define MCC_S_NICE_SEPARATE_RESP 52854801
#define MCC_S_MORE 52854809
#define MCC_S_DONE 52854817
#define MCC_S_CANCELED 52854825
#define MCC_S_CREATED 52854833
#define MCC_S_ALRDYEXISTS 52854841
#define MCC_S_TESTSUCCESS 52854849
#define MCC_S_RESPONSE 52854857
#define MCC_S_ILVIMPLDEFAULT 52854865
#define MCC_S_COMMON_EXCEPTION 52854873
#define MCC_S_SPECIALIZED_EXCEPTION 52854881
#define MCC_S_EVENTREPORT 52854889
#define MCC_S_NULL_INSTANCE 52854897
#define MCC_S_DNS_UPDATED 52854905
#define MCC_S_INDSET 52854913
#define MCC_S_INDDEF 52854921
#define MCC_S_DEFQUALCLR 52854929
#define MCC_S_DEFQUAL 52854937
#define MCC_S_DEFENTCLR 52854945
#define MCC_S_DEFENT 52854953
#define MCC_S_SAVED 52854961
#define MCC_S_PRINTED 52854969
#define MCC_S_NOTIFDONE 52854977
#define MCC_S_NOTIFTERM 52854985
#define MCC_S_VERIFYENTER 52854993
#define MCC_S_VERIFYEXIT 52855001
#define MCC_S_FILCREATED 52858795
#define MCC_S_NOT_IMPLEMENTED 52858803
#define MCC_S_DUPLICATE_ENTRY 52858811
#define MCC_S_CREATING_GS 52858819
#define MCC_S_NO_INFO_RTND 52858827
#define MCC_S_DECNET_EMM_LOG 52858835
#define MCC_S_MESSAGE_MSG 52858843
#define MCC_S_PROTOCOL_MSG 52858851
#define MCC_S_REQ_NOT_SATIS 52858859
#define MCC_S_TEST 52858867
#define MCC_S_THRCREATE 52858875
#define MCC_S_THRSHW 52858883
#define MCC_S_THRSHWHD 52858891
#define MCC_S_RNBSHWHD 52858899
#define MCC_S_RNBSHW 52858907
#define MCC_S_COULKCRE 52858915
#define MCC_S_COULKDEL 52858923
#define MCC_S_COULKREL 52858931
#define MCC_S_COULKAPR 52858939
#define MCC_S_COULKAEX 52858947
#define MCC_S_COULKHKF1 52858955
#define MCC_S_COULKHKF2 52858963
#define MCC_S_LKBSHFUL1 52858971
#define MCC_S_SHTHRSTAT1 52858979
#define MCC_S_SHTHRSTAT2 52858987
#define MCC_S_SHTHRSTAT3 52858995
#define MCC_S_SHTHRSTAT4 52859003
#define MCC_S_THRCALLS 52859011
#define MCC_S_ENDCMD 52859019
#define MCC_S_NOCMD 52859027
#define MCC_S_INVALIDAG 52859035
#define MCC_S_INVVERB 52859043
#define MCC_S_INVENTITY 52859051
#define MCC_S_XEVENT 52859059
#define MCC_S_XEQCREATED 52859067
#define MCC_S_XASSIGN 52859075
#define MCC_S_GETNEXTEVENT 52859083
#define MCC_S_RCVNEXTEVENT 52859091
#define MCC_S_NQEVENT 52859099
#define MCC_S_THRTERMINATION 52859107
#define MCC_S_SHTHRSTAT1A 52859115
#define MCC_S_FWINIT 52859123
#define MCC_S_FWTERM 52859131
#define MCC_S_DEBUG_EVENT 52859139
#define MCC_S_LOG_EVENT 52859147
#define MCC_S_LOGENTRY_HDR 52859155
#define MCC_S_RTN_ENTRY 52859163
#define MCC_S_RTN_RETURN 52859171
#define MCC_S_TRBSHOW 52859179
#define MCC_S_ALERTRECVD 52859187
#define MCC_S_LOGFILHDR 52859195
#define MCC_S_IDDIR_EXT 52859203
#define MCC_S_INPROG 52859211
#define MCC_S_PENDING 52859219
#define MCC_S_COUSEMHKF1 52859227
#define MCC_S_COUSEMHKF2 52859235
#define MCC_S_SEMSHWHD 52859243
#define MCC_S_SEMSHW 52859251
#define MCC_S_MUTEXSHWHD 52859259
#define MCC_S_MUTEXSHW 52859267
#define MCC_S_MUTEXCRE 52859275
#define MCC_S_MUTEXDEL 52859283
#define MCC_S_SENDALERT 52859291
#define MCC_S_NOTIME 52859299
#define MCC_S_FIELD_XL 52859307
#define MCC_S_FIELD_QU 52859315
#define MCC_S_FIELD_AC 52859323
#define MCC_S_FIELD_T 52859331
#define MCC_S_FIELD_SL 52859339
#define MCC_S_FIELD_ENUM 52859347
#define MCC_S_FIELD_CVR 52859355
#define MCC_S_OBJECT 52859363
#define MCC_S_EVENT 52859371
#define MCC_S_ASTCOUNT 52859379
#define MCC_S_TESTCOUNT 52859387
#define MCC_S_VMSLOCK_AST 52859395
#define MCC_S_SHTHRSTAT5 52859403
#define MCC_S_REPOSRECOVER 52859411
#define MCC_S_CREATING_MIR 52859419
#define MCC_S_FIELD_AF 52859427
#define MCC_S_COUNTER 52859435
#define MCC_S_LOCKABORT 52859443
#define MCC_S_VMSLOCK_ACQUIRE 52859451
#define MCC_S_VMSLOCK_BLOCKING_AST 52859459
#define MCC_S_VMSLOCK_DELETE 52859467
#define MCC_S_VMSLOCK_RELEASE 52859475
#define MCC_S_CANCEL 52859483
#define MCC_S_P1SV_STATE 52859491
#define MCC_S_FW_SWITCHES 52859499
#define MCC_S_CHILDWIDGET 52859507
#define MCC_S_WIDGET_ORDER 52859515
#define MCC_S_WIDGET_ASSIGNED 52859523
#define MCC_S_WIDGETCACHEHIT 52859531
#define MCC_S_XDEASSIGN 52859539
#define MCC_S_XEQ 52859547
#define MCC_S_XEVENTDISCARD 52859555
#define MCC_S_XEQCOUNT 52859563
#define MCC_S_XPENDING 52859571
#define MCC_S_MUTEXUNLOCK 52859579
#define MCC_S_MUTEXLOCK 52859587
#define MCC_S_CVSHWHD 52859595
#define MCC_S_CVSHW 52859603
#define MCC_S_CVBRDCAST 52859611
#define MCC_S_CVCRE 52859619
#define MCC_S_CVDEL 52859627
#define MCC_S_CVSIGNAL 52859635
#define MCC_S_CVWAIT 52859643
#define MCC_S_XSEMSHWHD 52859651
#define MCC_S_XSEMSHW 52859659
#define MCC_S_NOEVENTREC 52859667
#define MCC_S_NOEVENTREQ 52859675
#define MCC_S_CREATING_EVT 52859683
#define MCC_S_QUENOWMT 52859691
#define MCC_S_NOSYMINTBL 52859699
#define MCC_S_GSTBLUNMAP 52859707
#define MCC_S_DSPTBLCREATED 52859715
#define MCC_S_ENRDUPLENTRY 52859723
#define MCC_S_INDMAX 52859731
#define MCC_S_INDMIN 52859739
#define MCC_S_SAVEHEADER 52859747
#define MCC_S_SAVETRAILER 52859755
#define MCC_S_NOHCLASS 52859763
#define MCC_S_NOHSUBCLASS 52859771
#define MCC_S_NOHCLASSVERB 52859779
#define MCC_S_NOHSUBCLASSVERB 52859787
#define MCC_S_PARTABIDREAD 52859795
#define MCC_S_PARTABIDEXP 52859803
#define MCC_S_PARTABBLT 52859811
#define MCC_S_SYMTABEMPTY 52859819
#define MCC_S_VERIFYISON 52859827
#define MCC_S_VERIFYISOFF 52859835
#define MCC_S_LOGGINGISON 52859843
#define MCC_S_LOGGINGISOFF 52859851
#define MCC_S_NONOTIFY 52859859
#define MCC_S_NOTIFYHEAD1 52859867
#define MCC_S_NOTIFYHEAD2 52859875
#define MCC_S_NOTIFYHEAD3 52859883
#define MCC_S_REPOSEXIST 52859891
#define MCC_S_PM_W_MSG 52859899
#define MCC_S_PM_EM_MSG 52859907
#define MCC_S_PM_MP_MSG 52859915
#define MCC_S_PM_ALREADYCRE 52859923
#define MCC_S_PM_MEMALREADYCRE 52859931
#define MCC_S_PM_DOMALREADYCRE 52859939
#define MCC_S_PM_MAPQUERY_INUSE 52859947
#define MCC_S_PM_MAPFIND_INUSE 52859955
#define MCC_S_PM_NOMULTICLASSOPER 52859963
#define MCC_S_PM_NODELDEREGDIR 52859971
#define MCC_S_PM_DOMAIN_MEMBER_ERROR 52859979
#define MCC_S_PM_ENTITY_EXPANDED 52859987
#define MCC_S_PM_DOMAIN_MEMBER_DEREG 52859995
#define MCC_S_PM_UNKNOWNCLASS 52860003
#define MCC_S_PM_PL_MSG 52860011
#define MCC_S_SYMDEF 52860019
#define MCC_S_DEFENTHEAD 52860027
#define MCC_S_DEFQUALHEAD 52860035
#define MCC_S_DEFPROMPTHEAD 52860043
#define MCC_S_DEFINDENTHEAD 52860051
#define MCC_S_MCCPROMPT 52860059
#define MCC_S_CLIHELPNAME 52860067
#define MCC_S_PRESSRTN 52860075
#define MCC_S_UNKNOWN_MBXMSG 52866792
#define MCC_S_OPTDATA_NOTSENT 52866800
#define MCC_S_INCOMPLETE 52866808
#define MCC_S_NOMATCH 52866816
#define MCC_S_INVEXPR 52866824
#define MCC_S_BADPOSTFIX 52866832
#define MCC_S_DEADLOCK 52866840
#define MCC_S_EMPTYQUEUE 52866848
#define MCC_S_IVLOCKID 52866856
#define MCC_S_IVLOCKCONV 52866864
#define MCC_S_VERBSYNTAX 52866872
#define MCC_S_VERBAMBIG 52866880
#define MCC_S_ENTMISSING 52866888
#define MCC_S_INVALIDENT 52866896
#define MCC_S_VEINVALID 52866904
#define MCC_S_CLASSAMBIG 52866912
#define MCC_S_NODEFVERB 52866920
#define MCC_S_CLASSSYNTAX 52866928
#define MCC_S_INSTANCESYNTAX 52866936
#define MCC_S_QUOTESYNTAX 52866944
#define MCC_S_INSTANCEMISSING 52866952
#define MCC_S_INVATTR 52866960
#define MCC_S_ATTRAMBIG 52866968
#define MCC_S_VEAINVALID 52866976
#define MCC_S_EOSEXPECTED 52866984
#define MCC_S_VALUEMISSING 52866992
#define MCC_S_ATTRSYNTAX 52867000
#define MCC_S_UNSUPPINTSIZE 52867008
#define MCC_S_UNSUPPUNSSIZE 52867016
#define MCC_S_RULEEXPDT 52867024
#define MCC_S_ENTNAMEDT 52867032
#define MCC_S_SETDT 52867040
#define MCC_S_NODEFDATA 52867048
#define MCC_S_NOATTRNAME 52867056
#define MCC_S_REQCAN 52867064
#define MCC_S_NOFMMDATA 52867072
#define MCC_S_VERBUNKNOWN 52867080
#define MCC_S_ENTUNKNOWN 52867088
#define MCC_S_ATTRUNKNOWN 52867096
#define MCC_S_ARGUNKNOWN 52867104
#define MCC_S_ARGAMBIG 52867112
#define MCC_S_ARGSYNTAX 52867120
#define MCC_S_ATTRVEAINV 52867128
#define MCC_S_ARGVEAINV 52867136
#define MCC_S_INVARG 52867144
#define MCC_S_ATTRVALUEMISS 52867152
#define MCC_S_ARGVALUEMISS 52867160
#define MCC_S_ATTRVALUESYNTAX 52867168
#define MCC_S_ARGVALUESYNTAX 52867176
#define MCC_S_ATTRVALNOTALLOW 52867184
#define MCC_S_ARGVALNOTALLOW 52867192
#define MCC_S_ATTACHNOARG 52867200
#define MCC_S_ENROLLNOARG 52867208
#define MCC_S_DONOARG 52867216
#define MCC_S_EXITNOARG 52867224
#define MCC_S_DEFAULTAG 52867232
#define MCC_S_ATTRMISSING 52867240
#define MCC_S_ARGMISSING 52867248
#define MCC_S_QUALUNKNOWN 52867256
#define MCC_S_QUALAMBIG 52867264
#define MCC_S_QUALSYNTAX 52867272
#define MCC_S_COMMAMISSING 52867280
#define MCC_S_TIME_ALREADY_PASSED 52867288
#define MCC_S_ENT_INCOMPLETE 52867296
#define MCC_S_NO_PARENT 52867304
#define MCC_S_REPOSNOATTR 52867312
#define MCC_S_REPOSNONAME 52867320
#define MCC_S_REPOSISEMPTY 52867328
#define MCC_S_VMSLOCK_DEADLOCK 52867336
#define MCC_S_ASTSDISABLED 52867344
#define MCC_S_CALLWITHCANCEL 52867352
#define MCC_S_DSPTBLNOTUPD 52867360
#define MCC_S_ENRDUPLENTNOTEQ 52867368
#define MCC_S_ENRDSPTBLNOTUPD 52867376
#define MCC_S_ENRCOMPWARN 52867384
#define MCC_S_INDHIGH 52867392
#define MCC_S_INDLOW 52867400
#define MCC_S_CMDTRUNC 52867408
#define MCC_S_SYMSUBTRUNC 52867416
#define MCC_S_BATCHNOFORM 52867424
#define MCC_S_FORMSONLY 52867432
#define MCC_S_SAVERR 52867440
#define MCC_S_PRINTERR 52867448
#define MCC_S_PARTABIDMINMIS 52867456
#define MCC_S_INVEVENT 52867464
#define MCC_S_EVENTAMBIG 52867472
#define MCC_S_EVENTSYNTAX 52867480
#define MCC_S_EVENTUNKNOWN 52867488
#define MCC_S_EVENTVEAINV 52867496
#define MCC_S_EVENTMISSING 52867504
#define MCC_S_NOTIFABORT 52867512
#define MCC_S_SYMREQ 52867520
#define MCC_S_INVLOGDIRE 52867528
#define MCC_S_INVVERDIRE 52867536
#define MCC_S_NOTIFIDREQ 52867544
#define MCC_S_NOTIFIDINV 52867552
#define MCC_S_NOTIFNOTFND 52867560
#define MCC_S_NOCONSTRWITH 52867568
#define MCC_S_NOMULTPARTEVENT 52867576
#define MCC_S_USINGFIRSTPART 52867584
#define MCC_S_PM_NO_MEMBERS 52867592
#define MCC_S_PM_NO_CHILDREN 52867600
#define MCC_S_PM_NOTIF_UNAVAIL 52867608
#define MCC_S_ASNEXPOUTOFRANGE 52867616
#define MCC_S_ILVNOCLOSE 52867624
#define MCC_S_ILVINVVALUE 52867632
#define MCC_S_ILVPASTLIMIT 52867640
#define MCC_S_PARTABPREPNTNUL 52867648
#define MCC_S_PM_CHANGEPROPNOTSENT 52867656
#define MCC_S_INV_NCB 52874794
#define MCC_S_INV_DESC 52874802
#define MCC_S_INV_NETNAM 52874810
#define MCC_S_INV_HANDLE 52874818
#define MCC_S_INV_MULT_REQ 52874826
#define MCC_S_MUSTBETIMENOW 52874834
#define MCC_S_NODIAGNOSE 52874842
#define MCC_S_ID802SNAP_ERROR 52874850
#define MCC_S_ID802SAP_ERROR 52874858
#define MCC_S_ID802_ERROR 52874866
#define MCC_S_IDENETV2TYPE_ERROR 52874874
#define MCC_S_EXTRACOMMA 52874882
#define MCC_S_BADDATA 52874890
#define MCC_S_NODATARESPONSE 52874898
#define MCC_S_FULLENTITY_ERROR 52874906
#define MCC_S_LOCALENTITY_ERROR 52874914
#define MCC_S_NFSEOB 52874922
#define MCC_S_NOQUOTA 52874930
#define MCC_S_NOPROT 52874938
#define MCC_S_INVFTYPE 52874946
#define MCC_S_INVFSCOPE 52874954
#define MCC_S_FILENOTOPEN 52874962
#define MCC_S_ENTEXIST 52874970
#define MCC_S_NOPARENT 52874978
#define MCC_S_NORECORD 52874986
#define MCC_S_NOCHILD 52874994
#define MCC_S_INSUFBUFFER 52875002
#define MCC_S_INVALIDATTR 52875010
#define MCC_S_NOENTITY 52875018
#define MCC_S_INVALIDTIME 52875026
#define MCC_S_NOOPENQUOTA 52875034
#define MCC_S_ATTREXIST 52875042
#define MCC_S_RELEXIST 52875050
#define MCC_S_NOATTREXIST 52875058
#define MCC_S_NORELEXIST 52875066
#define MCC_S_FILEINUSE 52875074
#define MCC_S_RECORDINUSE 52875082
#define MCC_S_LOGNOTDEF 52875090
#define MCC_S_INVPARAM 52875098
#define MCC_S_TIMEOUT 52875106
#define MCC_S_ILVIMPLICIT 52875114
#define MCC_S_ILVNOTATVALUE 52875122
#define MCC_S_ILVNOTYPEFOUND 52875130
#define MCC_S_ILVSYNTAXERROR 52875138
#define MCC_S_ILVBUILDORPARSE 52875146
#define MCC_S_ILVNOTINCONS 52875154
#define MCC_S_ILVENCODING 52875162
#define MCC_S_ILVBUFTOOBIG 52875170
#define MCC_S_ILVTNF 52875178
#define MCC_S_ILVEOC 52875186
#define MCC_S_ILVNOTCONS 52875194
#define MCC_S_ILVTOOBIG 52875202
#define MCC_S_ILVMISTYPED 52875210
#define MCC_S_ILVTAG 52875218
#define MCC_S_ILVALREADYDONE 52875226
#define MCC_S_ILVNORTSQRBRKT 52875234
#define MCC_S_ILVTOOMNYRPAREN 52875242
#define MCC_S_ILVUNRECTOKEN 52875250
#define MCC_S_ILVWANTEDTAG 52875258
#define MCC_S_ILVWANTEDVALUE 52875266
#define MCC_S_NODATA 52875274
#define MCC_S_NOTFOUND 52875282
#define MCC_S_OVERFLOW 52875290
#define MCC_S_BADATTRG 52875298
#define MCC_S_BADVERB 52875306
#define MCC_S_BADDSPTBL 52875314
#define MCC_S_BADCLASS 52875322
#define MCC_S_BADINSTANCE 52875330
#define MCC_S_VERSION_SKEW 52875338
#define MCC_S_ENROLL_FAILED 52875346
#define MCC_S_REMOVE_FAILED 52875354
#define MCC_S_GS_CREATE_ERROR 52875362
#define MCC_S_BAD_BLOCK_PTR 52875370
#define MCC_S_BAD_BLOCK_TYPE 52875378
#define MCC_S_ENTITY_ID_NOT_RTND 52875386
#define MCC_S_NODE_ID_NOT_FND 52875394
#define MCC_S_ENTITY_ID_NOT_FND 52875402
#define MCC_S_MEM_ALLOC_FAILED 52875410
#define MCC_S_INVALIDCLASS 52875418
#define MCC_S_OUTOFMEM 52875426
#define MCC_S_INSUFBUF_ENTITY 52875434
#define MCC_S_INSUFBUF_OUT_P 52875442
#define MCC_S_UNKN_DECNET_PHASE 52875450
#define MCC_S_UNSUPPORTEDVERB 52875458
#define MCC_S_UNSUPPORTEDENTITY 52875466
#define MCC_S_UNSUPPORTEDTIME 52875474
#define MCC_S_TRANSMITERROR 52875482
#define MCC_S_RECEIVEERROR 52875490
#define MCC_S_GETCHANERROR 52875498
#define MCC_S_ENTITYEXISTS 52875506
#define MCC_S_NOENTITYEXISTS 52875514
#define MCC_S_INVALIDVALUE 52875522
#define MCC_S_INVALIDINSTANCE 52875530
#define MCC_S_NOPARAMLIST 52875538
#define MCC_S_INSRES 52875546
#define MCC_S_TERMREQ 52875554
#define MCC_S_ENT_MISMATCH 52875562
#define MCC_S_NO_OUTPUT_BUF 52875570
#define MCC_S_INVTHRID 52875578
#define MCC_S_ENDOFLIST 52875586
#define MCC_S_ASSERTFAIL 52875594
#define MCC_S_KERNINITF 52875602
#define MCC_S_GS_POOL_CORRUPTED 52875610
#define MCC_S_INSVIRMEM 52875618
#define MCC_S_UNEXPECTEDVALUE 52875626
#define MCC_S_UNEXPECTEDSTATUS 52875634
#define MCC_S_ALLOC 52875642
#define MCC_S_NOPARSER 52875650
#define MCC_S_INVDATAOI 52875658
#define MCC_S_NOATTRVALUE 52875666
#define MCC_S_DTNOTSUP 52875674
#define MCC_S_INVTIME 52875682
#define MCC_S_NOACCESSFMMDATA 52875690
#define MCC_S_FINDUSYM 52875698
#define MCC_S_ALERT_ABORT 52875706
#define MCC_S_ALERT_TERMINATE 52875714
#define MCC_S_CFGNOREGISTER 52875722
#define MCC_S_CFGDEREGFAIL 52875730
#define MCC_S_CFGDIRFAIL 52875738
#define MCC_S_CFGSETREFFAIL 52875746
#define MCC_S_CFGLONGEOUT 52875754
#define MCC_S_CFGNOWILD 52875762
#define MCC_S_LPARNFNDINEXP 52875770
#define MCC_S_RPARNFNDINEXP 52875778
#define MCC_S_NOENTITYINEXP 52875786
#define MCC_S_NOATTRINEXP 52875794
#define MCC_S_NORELOPINEXP 52875802
#define MCC_S_VALDTERRINEXP 52875810
#define MCC_S_UNSUPDTINEXP 52875818
#define MCC_S_INVVALINEXP 52875826
#define MCC_S_SVCUNDEF 52875834
#define MCC_S_BADTEST 52875842
#define MCC_S_INVOBJTYPE 52875850
#define MCC_S_WRONGNUMARGS 52875858
#define MCC_S_EXISTENCE_ERROR 52875866
#define MCC_S_INVPAS_ID 52875874
#define MCC_S_FREEVM_ERR 52875882
#define MCC_S_GETVM_ERR 52875890
#define MCC_S_REQARG 52875898
#define MCC_S_INVPRI 52875906
#define MCC_S_INVRMSASTPRM 52875914
#define MCC_S_INVSEMNAM 52875922
#define MCC_S_INVSEMMAX 52875930
#define MCC_S_INVSEMINIT 52875938
#define MCC_S_IN_USE_ERROR 52875946
#define MCC_S_SEMALRMAX 52875954
#define MCC_S_NOTLOCKOWNER 52875962
#define MCC_S_BADTEST2 52875970
#define MCC_S_MISSINGFILEKW 52875978
#define MCC_S_INSUF_BUF 52875986
#define MCC_S_NOOUT_ENT 52875994
#define MCC_S_NONEX_ENT 52876002
#define MCC_S_NO_ENTFILT 52876010
#define MCC_S_NO_WILDCARD 52876018
#define MCC_S_INV_SCOPE 52876026
#define MCC_S_INV_SCHEDULE 52876034
#define MCC_S_INV_VERB 52876042
#define MCC_S_INV_IN_ENTITY 52876050
#define MCC_S_INV_ATTRIBUTE 52876058
#define MCC_S_INV_TIME_SPEC 52876066
#define MCC_S_INV_IN_P 52876074
#define MCC_S_INV_IN_Q 52876082
#define MCC_S_INV_OUT_ENTITY 52876090
#define MCC_S_INV_TIME_STAMP 52876098
#define MCC_S_INV_OUT_P 52876106
#define MCC_S_INV_OUT_Q 52876114
#define MCC_S_INV_ENR_ID 52876122
#define MCC_S_INV_COMPONENT 52876130
#define MCC_S_INV_PERIODICITY 52876138
#define MCC_S_INV_PERIODEND 52876146
#define MCC_S_TIMADD 52876154
#define MCC_S_TIMSUB 52876162
#define MCC_S_TIMMUL 52876170
#define MCC_S_TIMINF 52876178
#define MCC_S_TIMREL 52876186
#define MCC_S_TIMABS 52876194
#define MCC_S_TIMUNSUPPORTED 52876202
#define MCC_S_INV_HANDLE_STATE 52876210
#define MCC_S_INV_ENTITY 52876218
#define MCC_S_NO_SUCH_CIPAIR 52876226
#define MCC_S_INVOBJ 52876234
#define MCC_S_INVQUE 52876242
#define MCC_S_INVTHRATTID 52876250
#define MCC_S_TBLTOOSMALL 52876258
#define MCC_S_INVREPOS 52876266
#define MCC_S_REPOSEXIST_OBS 52876274
#define MCC_S_NOREPOSEXIST 52876282
#define MCC_S_WRONGREPOSTYPE 52876290
#define MCC_S_IO_ERROR 52876298
#define MCC_S_INVRTYPE 52876306
#define MCC_S_CMDPROCNFND 52876314
#define MCC_S_ILLCMDPROC 52876322
#define MCC_S_NOCMDPROCSPEC 52876330
#define MCC_S_CTXKEY_ALLOC_FAIL 52876338
#define MCC_S_INV_CTXKEY 52876346
#define MCC_S_ENROLL_FULL 52876354
#define MCC_S_TIMINVBEGIN 52876362
#define MCC_S_TIMDUPBEGIN 52876370
#define MCC_S_LMF_FAILED 52876378
#define MCC_S_BADPROCLM 52876386
#define MCC_S_INVID 52876394
#define MCC_S_DEFNOTREGISTRD 52876402
#define MCC_S_DEFILVPARSEERR 52876410
#define MCC_S_UNSUPP_OP 52876418
#define MCC_S_CANTTESTEQUAL 52876426
#define MCC_S_INV_INSTDESC 52876434
#define MCC_S_INV_WILD 52876442
#define MCC_S_INV_COMPARE 52876450
#define MCC_S_NO_CLASS_GIVEN 52876458
#define MCC_S_NO_INSTANCE_ID 52876466
#define MCC_S_INVOPR 52876474
#define MCC_S_NOSECPROCSPEC 52876482
#define MCC_S_HANDLE_WAS_CANCELLED 52876490
#define MCC_S_INACCESS_ENT 52876498
#define MCC_S_ILV_BADLISTTUPLE 52876506
#define MCC_S_NOLOGDEF 52876514
#define MCC_S_BOOLEAN_ERROR 52876522
#define MCC_S_OCTET_ERROR 52876530
#define MCC_S_OCTET_LENGTH 52876538
#define MCC_S_OCTET_STRING_ERROR 52876546
#define MCC_S_OCTET_STRING_ODD 52876554
#define MCC_S_OCTET_STRING_PREFIX 52876562
#define MCC_S_HEX_STRING_ERROR 52876570
#define MCC_S_HEX_STRING_PREFIX 52876578
#define MCC_S_INTEGER8_ERROR 52876586
#define MCC_S_INTEGER16_ERROR 52876594
#define MCC_S_INTEGER32_ERROR 52876602
#define MCC_S_INTEGER64_ERROR 52876610
#define MCC_S_UNSIGNED8_ERROR 52876618
#define MCC_S_UNSIGNED16_ERROR 52876626
#define MCC_S_UNSIGNED32_ERROR 52876634
#define MCC_S_UNSIGNED64_ERROR 52876642
#define MCC_S_COUNTER16_ERROR 52876650
#define MCC_S_COUNTER32_ERROR 52876658
#define MCC_S_COUNTER48_ERROR 52876666
#define MCC_S_COUNTER64_ERROR 52876674
#define MCC_S_LCOUNTER16_ERROR 52876682
#define MCC_S_LCOUNTER32_ERROR 52876690
#define MCC_S_EXPRESSION_PAREN 52876698
#define MCC_S_VERSION_ERROR 52876706
#define MCC_S_ADDRESS_TSEL_LENGTH 52876714
#define MCC_S_ADDRESS_TSEL_ERROR 52876722
#define MCC_S_ADDRESS_DTE_LENGTH 52876730
#define MCC_S_ADDRESS_DTE_ERROR 52876738
#define MCC_S_PHASE4NAME_LENGTH 52876746
#define MCC_S_PHASE4NAME_ERROR 52876754
#define MCC_S_PHASE4ADDRESS_NOPERIOD 52876762
#define MCC_S_PHASE4ADDRESS_AREA 52876770
#define MCC_S_PHASE4ADDRESS_NUMBER 52876778
#define MCC_S_PHASE4ADDRESS_ERROR 52876786
#define MCC_S_UNBAL_OPENCLOSE 52876794
#define MCC_S_INVALID_OPEN 52876802
#define MCC_S_NO_CLOSE_DELIM 52876810
#define MCC_S_NO_OPEN_DELIM 52876818
#define MCC_S_SPLIT_CONSLIST 52876826
#define MCC_S_RECORD_FIELD_ERROR 52876834
#define MCC_S_RECORD_FIELD_NAME 52876842
#define MCC_S_ENUMERATION_AMBIG 52876850
#define MCC_S_ENUMERATION_ERROR 52876858
#define MCC_S_BIN_ABS_TIM_ERROR 52876866
#define MCC_S_BIN_REL_TIM_ERROR 52876874
#define MCC_S_RANGE_PERIODS 52876882
#define MCC_S_SUBRANGE_ERROR 52876890
#define MCC_S_OUTOFSUBRANGE 52876898
#define MCC_S_INVALID_BASETYPE 52876906
#define MCC_S_BITSET_ERROR 52876914
#define MCC_S_SIMPLENAME_ERROR 52876922
#define MCC_S_LATIN1STRING_ERROR 52876930
#define MCC_S_FILE_SPEC_ERROR 52876938
#define MCC_S_DIRECTORY_SPEC_ERROR 52876946
#define MCC_S_MCCERROR_ERROR 52876954
#define MCC_S_VMSERROR_ERROR 52876962
#define MCC_S_RANGE_ERROR 52876970
#define MCC_S_INVALIDIDENT 52876978
#define MCC_S_WITH_NOVALUE 52876986
#define MCC_S_WITH_NEEDPAIR 52876994
#define MCC_S_INVALIDRELOP 52877002
#define MCC_S_INVALIDLOGOP 52877010
#define MCC_S_QUALNAMEUNKNOWN 52877018
#define MCC_S_QUALNAMEAMBIG 52877026
#define MCC_S_ATSYNTAXERR 52877034
#define MCC_S_EXPRESSION_ERROR 52877042
#define MCC_S_CANTPOP 52877050
#define MCC_S_INVDICSTR 52877058
#define MCC_S_NOTINIT 52877066
#define MCC_S_NOTNESTEDCT 52877074
#define MCC_S_UFT 52877082
#define MCC_S_GS_POOL_VERSKEW 52877090
#define MCC_S_GS_POOL_NOTMAPPED 52877098
#define MCC_S_SINGLEEVERY 52877106
#define MCC_S_SINGLEUNTIL 52877114
#define MCC_S_NOTINPUTDT 52877122
#define MCC_S_OUTCONVERR 52877130
#define MCC_S_REPOSNORECOV 52877138
#define MCC_S_TOOMANYAST 52877146
#define MCC_S_IDDIR_CORRUPT 52877154
#define MCC_S_IDSPACEFULL 52877162
#define MCC_S_OPINV 52877170
#define MCC_S_ALREADYINIT 52877178
#define MCC_S_INVALID_SYNTAX_ERROR 52877186
#define MCC_S_DATA_ENCODING_ERROR 52877194
#define MCC_S_ATTRNOTALLOW 52877202
#define MCC_S_ABORTCTRLY 52877210
#define MCC_S_UNEXP_CHAR 52877218
#define MCC_S_ALERT_TERMREQ 52877226
#define MCC_S_REVECTCONFLICT 52877234
#define MCC_S_INVWIDGET_ID 52877242
#define MCC_S_WIDGET_ALRASN 52877250
#define MCC_S_WIDGET_NOTASN 52877258
#define MCC_S_XEQ_EXISTERR 52877266
#define MCC_S_INVDISPLAY_ID 52877274
#define MCC_S_WIDGET_NAVIG_FAIL 52877282
#define MCC_S_NOPHYIO 52877290
#define MCC_S_NOLOGIO 52877298
#define MCC_S_NONETDEV 52877306
#define MCC_S_PROTOCOLINUSE 52877314
#define MCC_S_INVDEVNAM 52877322
#define MCC_S_ENDUSERSPEC_ERROR 52877330
#define MCC_S_FILESPEC_ERROR 52877338
#define MCC_S_UIC_ERROR 52877346
#define MCC_S_FULLNAME_ERROR 52877354
#define MCC_S_UID_ERROR 52877362
#define MCC_S_USE_ERROR 52877370
#define MCC_S_SINGLESTART 52877378
#define MCC_S_SINGLEDUR 52877386
#define MCC_S_DNSERROR 52877394
#define MCC_S_NOATTRIB 52877402
#define MCC_S_NAMETOOLONG 52877410
#define MCC_S_NOTXSEMOWNER 52877418
#define MCC_S_EVENT_LOST 52877426
#define MCC_S_INSEVTPOOLMEM 52877434
#define MCC_S_EVT_CREATE_ERROR 52877442
#define MCC_S_EVT_BAD_BLOCK_PTR 52877450
#define MCC_S_EVT_BAD_BLOCK_TYPE 52877458
#define MCC_S_EVT_POOL_NOTMAPPED 52877466
#define MCC_S_EVT_MALLOC_FAILED 52877474
#define MCC_S_EVT_BAD_RMB 52877482
#define MCC_S_EVT_REMQHI_FAILED 52877490
#define MCC_S_NOMOREVT 52877498
#define MCC_S_ILV_BAD_MODE 52877506
#define MCC_S_ILV_NOSUCHMODE 52877514
#define MCC_S_INCONSISTENTCLASS 52877522
#define MCC_S_UNABLETOOPEN 52877530
#define MCC_S_NOSUCHSYMB 52877538
#define MCC_S_PRMPTNOTSET 52877546
#define MCC_S_INVMODEDIRE 52877554
#define MCC_S_INVDISPDIRE 52877562
#define MCC_S_INV_DNSIDENT 52877570
#define MCC_S_DUP_IDENTIFIER 52877578
#define MCC_S_INVGSNAM 52877586
#define MCC_S_DSPTBLNOTCRE 52877594
#define MCC_S_DSPTBLNOPRV 52877602
#define MCC_S_DSPTBLFNF 52877610
#define MCC_S_IPADDRESS_ERROR 52877618
#define MCC_S_INTERNETNAME_ERROR 52877626
#define MCC_S_NOPERIODICITY 52877634
#define MCC_S_TIMELEMISMATCH 52877642
#define MCC_S_ENRRTNNOTFND 52877650
#define MCC_S_ENRBADFILNAM 52877658
#define MCC_S_IDMISMATCH 52877666
#define MCC_S_INV_ENTINST_DT 52877674
#define MCC_S_WRONGNAME 52877682
#define MCC_S_INVUSEDIRCT 52877690
#define MCC_S_INVUSEDEFARG 52877698
#define MCC_S_SEQUENCE_ERROR 52877706
#define MCC_S_REAL_ERROR 52877714
#define MCC_S_DEFER_EVERYUNTIL 52877722
#define MCC_S_DSPTBLMEMCORR 52877730
#define MCC_S_DSPTBLFILCORR 52877738
#define MCC_S_DSPTBLVERSKEW 52877746
#define MCC_S_DSPTBLNOTMAPPED 52877754
#define MCC_S_DSPTBLDIRERR 52877762
#define MCC_S_DSPTBLDEVERR 52877770
#define MCC_S_DSPTBLFILERR 52877778
#define MCC_S_DSPTBLNOTACC 52877786
#define MCC_S_DSPMMLOCFILERR 52877794
#define MCC_S_ENRFILREADERR 52877802
#define MCC_S_ENRDSPTBLFNF 52877810
#define MCC_S_INVIND 52877818
#define MCC_S_DEFQUALERR 52877826
#define MCC_S_DEFENTERR 52877834
#define MCC_S_NODICTOBJ 52877842
#define MCC_S_INVSELATTR 52877850
#define MCC_S_NOVARSELINFO 52877858
#define MCC_S_EVENTNOTALLOW 52877866
#define MCC_S_NOTIFSTARTERR 52877874
#define MCC_S_NOTIFLOCK 52877882
#define MCC_S_NOTIFOUTPERR 52877890
#define MCC_S_NOTIFUNEXPRESP 52877898
#define MCC_S_NOTIFEXCP 52877906
#define MCC_S_NOTIFERR 52877914
#define MCC_S_NOEXCPTEXT 52877922
#define MCC_S_NORESPTEXT 52877930
#define MCC_S_NOTIFHNDLERR 52877938
#define MCC_S_NOTIFIOERR 52877946
#define MCC_S_DATATYPE_NOT_SUPPORTED 52877954
#define MCC_S_NOT_REGISTERED 52877962
#define MCC_S_PM_NO_AG_INFO 52877970
#define MCC_S_PM_NO_FOCUS 52877978
#define MCC_S_PM_MUST_SEND 52877986
#define MCC_S_PM_ENTER_FIRST 52877994
#define MCC_S_PM_VAL_ERROR 52878002
#define MCC_S_PM_REQ_ERROR 52878010
#define MCC_S_PM_VAL_REQ_ERROR 52878018
#define MCC_S_PM_DT_NOT_IMPL 52878026
#define MCC_S_PM_NO_ARGS 52878034
#define MCC_S_PM_VAL_NOT_RETURNED 52878042
#define MCC_S_PM_VALS_NOT_RETURNED 52878050
#define MCC_S_PM_NO_VALS_RETURNED 52878058
#define MCC_S_PM_ALL_DEFS_NO_SEND 52878066
#define MCC_S_PM_CONS_ERROR 52878074
#define MCC_S_PM_CONS_REQ_ERROR 52878082
#define MCC_S_PM_EXTRA_ELEMENTS 52878090
#define MCC_S_PM_VAR_REC_ERROR 52878098
#define MCC_S_PM_CHANGE_ONE_LEVEL 52878106
#define MCC_S_PM_INST_WILDCARD 52878114
#define MCC_S_PM_ATTR_FOUND 52878122
#define MCC_S_PM_NOT_IN_TBL 52878130
#define MCC_S_PM_INVALID_ENT 52878138
#define MCC_S_PM_CLASS_AMBIG 52878146
#define MCC_S_PM_CLASS_SYNTAX 52878154
#define MCC_S_PM_INST_SYNTAX 52878162
#define MCC_S_PM_QUOTE_SYNTAX 52878170
#define MCC_S_PM_INST_MISSING 52878178
#define MCC_S_PM_ADD_INUSE 52878186
#define MCC_S_PM_CTXNOTCOP 52878194
#define MCC_S_PM_SETCTXERR 52878202
#define MCC_S_PM_CREEMERR 52878210
#define MCC_S_PM_CTXNOTCRE 52878218
#define MCC_S_MAP_ENTNOTINDICT 52878226
#define MCC_S_MAP_NOSUBENTITIES 52878234
#define MCC_S_MAP_NODOMAIN 52878242
#define MCC_S_PM_FILOPENERR 52878250
#define MCC_S_PM_ENTNOTFND 52878258
#define MCC_S_PM_NOENTTOFND 52878266
#define MCC_S_PM_WINDOWMISMATCH 52878274
#define MCC_S_PM_DEWINDOWMISMATCH 52878282
#define MCC_S_PM_DRWINDOWMISMATCH 52878290
#define MCC_S_PM_BADFILEDATA 52878298
#define MCC_S_MAP_DSABLFADD 52878306
#define MCC_S_MAP_TDSABLFADD 52878314
#define MCC_S_MAP_DSABLFDELETE 52878322
#define MCC_S_MAP_TDSABLFDELETE 52878330
#define MCC_S_PM_INPUTERR 52878338
#define MCC_S_PM_NO_DOMAIN 52878346
#define MCC_S_PM_FILTERED 52878354
#define MCC_S_PM_AUGMENTED 52878362
#define MCC_S_PM_ADDNOARGS 52878370
#define MCC_S_PM_INV_DNSIDENT 52878378
#define MCC_S_ALRM_NO_DATA 52878386
#define MCC_S_ALRM_SINGLE_SELLIM 52878394
#define MCC_S_NT_STRUCT_CORR 52878402
#define MCC_S_NT_UNEXP 52878410
#define MCC_S_NT_STOPPED 52878418
#define MCC_S_PM_NODIRSELECTED 52878426
#define MCC_S_PM_NOATTRGRPSEL 52878434
#define MCC_S_PM_DEF_DOMAIN_EXIST_ERR 52878442
#define MCC_S_PM_PARSE_DEF_DOMAIN_ERR 52878450
#define MCC_S_PM_DOMAIN_LEN_ERR 52878458
#define MCC_S_CANT_GET_DOM_DATA 52878466
#define MCC_S_REASN_RECEIVED 52878474
#define MCC_S_PM_DCREICONERR 52878482
#define MCC_S_PM_DCRECONNECTIONERR 52878490
#define MCC_S_PM_DSCROLLERR 52878498
#define MCC_S_PM_DINITERR 52878506
#define MCC_S_PM_DDRAWERR 52878514
#define MCC_S_PM_DGODERR 52878522
#define MCC_S_PM_DTRANERR 52878530
#define MCC_S_PM_MAPSAVED 52878538
#define MCC_S_PM_MAPNOTSAVED 52878546
#define MCC_S_PM_CGENFORMNOTFETCH 52878554
#define MCC_S_PM_CMAPFORMNOTFETCH 52878562
#define MCC_S_PM_CMGMNTFORMNOTFETCH 52878570
#define MCC_S_PM_CINVWINDOWFLAG 52878578
#define MCC_S_PM_CMAPWIDTHTOOSMALL 52878586
#define MCC_S_PM_CMAPHEIGHTTOOSMALL 52878594
#define MCC_S_PM_CMAPNAVWIDTHTOOSMALL 52878602
#define MCC_S_PM_CMNGMTWIDTHTOOSMALL 52878610
#define MCC_S_PM_CMNGMTHEIGHTTOOSMALL 52878618
#define MCC_S_PM_CDIMENSIONTOOSMALL 52878626
#define MCC_S_PM_CINVMAPSHADOWING 52878634
#define MCC_S_PM_CINVGENFONT 52878642
#define MCC_S_PM_INVALIDCOLOR1 52878650
#define MCC_S_PM_INVALIDCOLOR2 52878658
#define MCC_S_PM_INVALIDCOLOR3 52878666
#define MCC_S_PM_INVALIDCOLOR4 52878674
#define MCC_S_PM_INVALIDCOLOR5 52878682
#define MCC_S_PM_INVALIDCOLOR6 52878690
#define MCC_S_PM_TIMEFORMNOTFETCH 52878698
#define MCC_S_PM_TIMEALREADYEXISTS 52878706
#define MCC_S_PM_TIMENOSTARTTIMEREP 52878714
#define MCC_S_PM_TIMENOSTARTTIMEDUR 52878722
#define MCC_S_PM_TIMENOREPINTERVAL 52878730
#define MCC_S_PM_TIMEINVALDAYSVAL1 52878738
#define MCC_S_PM_TIMEINVALDAYSVAL2 52878746
#define MCC_S_PM_TIMEINVALDAYSVAL3 52878754
#define MCC_S_PM_TIMEINVALDAYSVAL4 52878762
#define MCC_S_PM_TIMEINVALMONTHVAL 52878770
#define MCC_S_PM_TIMEINVALYEARVAL 52878778
#define MCC_S_PM_TIMEINVALHOURVAL 52878786
#define MCC_S_PM_TIMEINVALHOURVAL1 52878794
#define MCC_S_PM_TIMEINVALSECMINVAL 52878802
#define MCC_S_PM_SECONDSGTTEN 52878810
#define MCC_S_PM_CACFORMNOTFETCH 52878818
#define MCC_S_PM_CPATHFORMNOTFETCH 52878826
#define MCC_S_PM_NONAMESPACEALLOWED 52878834
#define MCC_S_PM_UNSUPP_FAO_NUM 52878842
#define MCC_S_ASNCORRUPT 52878850
#define MCC_S_MMABORT 52878858
#define MCC_S_PM_DSCALEERR 52878866
#define MCC_S_PM_NO_DOMAINS 52878874
#define MCC_S_WT_PROPACCESSERR 52878882
#define MCC_S_WT_NOSUCHAPPLID 52878890
#define MCC_S_WT_NOSUCHWINID 52878898
#define MCC_S_WT_PROPDATAMISMATCH 52878906
#define MCC_S_WT_NODOMAINNAME 52878914
#define MCC_S_WT_UNSUPPDOMTYPE 52878922
#define MCC_S_WT_INVALIDAPPLID 52878930
#define MCC_S_PM_UNSUPP_PROPTYPE 52878938
#define MCC_S_PM_REQNOTPROC 52878946
#define MCC_S_INSUF_PRIVS 52878954
#define MCC_S_ILVNOTIMPLYET 52880388
#define MCC_S_DBFATAL 52880396
#define MCC_S_FATAL 52880404
#define MCC_S_FAILED 52880412
#define MCC_S_NODICTIONARY 52880420
#define MCC_S_THRSTKOVF 52880428
#define MCC_S_THRSTK 52880436
#define MCC_S_GS_MALLOC_FAILED 52880444
#define MCC_S_INVALID_PARAMETER 52880452
#define MCC_S_TRM_FAILURE 52880460
#define MCC_S_INVALIDCMD 52880468
#define MCC_S_PTBINACC 52880476
#define MCC_S_FATAL_FW 52880484
#define MCC_S_INVALID_CVR 52880492
#define MCC_S_INVPROTOCOL 52880500
#define MCC_S_TOOMUCHDATA 52880508
#define MCC_S_INVALIDADDR 52880516
#define MCC_S_RELEASECHANER 52880524
#define MCC_S_STRTDEVERROR 52880532
#define MCC_S_STOPDEVERROR 52880540
#define MCC_S_UNKXMUTEOPER 52880548
#define MCC_S_RCVTIMEERROR 52880556
#define MCC_S_EVENTINTERNERR 52880564
#define MCC_S_EVT_POOL_CORRUPTED 52880572
#define MCC_S_EVT_POOL_VERSKEW 52880580
#define MCC_S_CC_INIT 52880588
#define MCC_S_CONDVAR_INIT 52880596
#define MCC_S_IDENTIFIER_INIT 52880604
#define MCC_S_FRAMEWORK_INIT 52880612
#define MCC_S_MUTEX_INIT 52880620
#define MCC_S_RMS_INIT 52880628
#define MCC_S_SEMAPHORE_INIT 52880636
#define MCC_S_SYNCH_INIT 52880644
#define MCC_S_THREAD_INIT 52880652
#define MCC_S_TIMER_INIT 52880660
#define MCC_S_TSS_INIT 52880668
#define MCC_S_XEVENT_INIT 52880676
#define MCC_S_XSEMAPHORE_INIT 52880684
#define MCC_S_DISPINTERNERR 52880692
#define MCC_S_PARTABOPEN 52880700
#define MCC_S_PARTABINSVIRMEM 52880708
#define MCC_S_PARTABREAD 52880716
#define MCC_S_PARTABIDMAJMIS 52880724
#define MCC_S_PARTABPREFIX 52880732
#define MCC_S_PARTABPREBAD 52880740
#define MCC_S_PARTABBODY 52880748
#define MCC_S_PARTABBODYBAD 52880756
#define MCC_S_MIR_INIT_FAIL 52880764
#define MCC_S_DICT_INIT_FAIL 52880772
#define MCC_S_TIME_INIT_FAIL 52880780
#define MCC_S_OBJ_INIT_FAIL 52880788
#define MCC_S_PM_BUG 52880796
#define MCC_S_PM_DRMTAGERR 52880804
#define MCC_S_NO_DOMSUBCLASS 52880812
#define MCC_S_PM_DECW_ERROR 52880820
#define MCC_S_PM_UNEXPECTED 52880828
#define MCC_S_PM_INVNUMENT 52880836
#define MCC_S_PM_FOPERERR 52880844
#define MCC_S_PM_FSWMAINWIN 52880852
#define MCC_S_PM_INITKITERR 52880860
#define MCC_S_PM_OPENHIERERR 52880868
#define MCC_S_PM_READPARERR 52880876
#define MCC_S_PM_INIMBOXERR 52880884
#define MCC_S_PM_THRGETERR 52880892
#define MCC_S_PM_NULEVTERR 52880900
#define MCC_S_PM_FSTWINERR 52880908
#define MCC_S_PM_REGCUSTERR 52880916
#define MCC_S_PM_REGMAPERR 52880924
#define MCC_S_PM_REGMGMNTERR 52880932
#define MCC_S_PM_NOADDOPER 52880940
#define MCC_S_FILNOTAVL 52880948
#define MCC_S_FILNOPRV 52880956
#define MCC_S_BADFILSPEC 52880964
#define MCC_S_FILNOTFND 52880972
#define MCC_S_PM_CANT_GET_VAR_INFO 52880980
#define MCC_S_PM_DICT_NO_CODE_ENTITY 52880988
#define MCC_S_PM_DICT_NO_CODE_DIRECTIVE 52880996
#define MCC_S_PM_DICT_NO_CODE_ATTG 52881004
#define MCC_S_PM_DICT_NO_PRES_ENTITY 52881012
#define MCC_S_PM_DICT_NO_PRES_ATTG 52881020
#define MCC_S_PM_DICT_NO_PRES_ATT_ARG 52881028
#define MCC_S_PM_DICT_NO_PRES_DIRECTIVE 52881036
#define MCC_S_PM_DICT_NO_DIRECTIVE_TYPE 52881044
#define MCC_S_PM_DICT_NO_ACCESS 52881052
#define MCC_S_PM_DICT_NO_DATATYPE 52881060
#define MCC_S_PM_DICT_NO_INSTANCE_REQ 52881068
#define MCC_S_PM_FB_LOAD_ERROR 52881076
#define MCC_S_PM_DRMFETCHERR 52881084
