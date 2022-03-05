/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#include <ciptypes.h>
#include <endianconv.h>
#include <trace.h>


const CipInstanceNum kCipInstanceNumMax = UINT16_MAX;


/* functions*/
size_t GetCipDataTypeLength(EipUint8 type, const EipUint8 *data) {

	size_t length = 0;

	switch (type) {
		case kCipBool:
		case kCipSint:
		case kCipUsint:
		case kCipByte:
			length = 1;
			break;

		case kCipInt:
		case kCipUint:
		case kCipWord:
		case kCipUsintUsint:
		case kCipItime:
		case kCipDate:
		case kCipEngUnit:
			length = 2;
			break;

		case kCipDint:
		case kCipUdint:
		case kCipDword:
		case kCipStime:
		case kCipFtime:
		case kCipTime:
		case kCipReal:
		case kCipTimeOfDay:
			length = 4;
			break;

		case kCipLint:
		case kCipUlint:
		case kCipLreal:
		case kCipLword:
		case kCipLtime:
			length = 8;
			break;

		case kCip6Usint:
			length = 6;
			break;

		case kCipString:
		case kCipString2:
		case kCipStringN:
			if(NULL != data){
				length = GetIntFromMessage(&data) + 2; // string length + 2 bytes length indicator
			}
			break;

		case kCipShortString:
			if(NULL != data){
				length = GetSintFromMessage(&data) + 1; // string length + 1 byte length indicator
			}
			break;

		case kCipEpath:
			if(NULL != data){
				length = GetIntFromMessage(&data) + 2; // path size + 2 bytes path size indicator
			}
			break;

		case kCipByteArray:
			if (NULL != data) {
				CipByteArray *byte_array = (CipByteArray*) data;
				length = byte_array->length;
			}
			break;

		default:
			OPENER_TRACE_ERR("GetCipDataTypeLength ERROR\n");
			return 0;

		/* TODO: missing data types:
		 * kCipAny
		 * kCipDateAndTime
		 * kCipStringI
		 * kCipMemberList
		 */
	}
	return length;
}
