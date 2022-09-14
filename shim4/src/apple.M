#ifndef IOS
#import <Cocoa/Cocoa.h>
#endif

#if defined IOS || defined MAS
#import <CloudKit/CloudKit.h>
#endif

#include "shim4/main.h"
#include "shim4/apple.h"
#include "shim4/util.h"

using namespace noo;

namespace noo {

namespace util {

std::string apple_get_system_language()
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

	NSString *str = [[NSLocale preferredLanguages] objectAtIndex:0];

	std::string result;
    
	if ([str hasPrefix:@"de"]) {
		result = "german";
	}
	else if ([str hasPrefix:@"fr"]) {
		result = "french";
	}
	else if ([str hasPrefix:@"nl"]) {
		result = "dutch";
	}
	else if ([str hasPrefix:@"el"]) {
		result = "greek";
	}
	else if ([str hasPrefix:@"it"]) {
		result = "italian";
	}
	else if ([str hasPrefix:@"pl"]) {
		result = "polish";
	}
	else if ([str hasPrefix:@"pt-BR"]) {
		result = "brazilian";
	}
	else if ([str hasPrefix:@"pt"]) {
		result = "portuguese";
	}
	else if ([str hasPrefix:@"ru"]) {
		result = "russian";
	}
	else if ([str hasPrefix:@"es"]) {
		result = "spanish";
	}
	else if ([str hasPrefix:@"ko"]) {
		result = "korean";
	}
	else {
		result = "english";
	}

	[pool release];

	return result;
}

#if defined IOS || defined MAS
static int cloud_error_code = 1;

static std::string cloud_container_name;
static std::string cloud_record_name;

void apple_set_cloud_info(std::string _cloud_container_name, std::string _cloud_record_name)
{
	cloud_container_name = _cloud_container_name;
	cloud_record_name = _cloud_record_name;
}

static CKDatabase *get_cloud_database()
{
	NSString *container_name = [NSString stringWithUTF8String:cloud_container_name.c_str()];
	CKContainer *myContainer = [CKContainer containerWithIdentifier:container_name];
	CKDatabase *publicDatabase = [myContainer publicCloudDatabase];
	return publicDatabase;
}

static CKRecord *get_cloud_record(CKDatabase *db)
{
	CKDatabase *publicDatabase = get_cloud_database();
	CKRecordID *myRecordID = [[CKRecordID alloc] initWithRecordName:[NSString stringWithUTF8String:cloud_record_name.c_str()]];
	__block CKRecord *r;
	__block bool done = false;
	[publicDatabase fetchRecordWithID:myRecordID completionHandler:^(CKRecord *myRecord, NSError *error) {
		if (error) {
			NSLog(@"%@", [error localizedDescription]);
			cloud_error_code = 2;
			r = nil;
			done = true;
		}
		else {
			r = myRecord;
			[r retain];
			done = true;
		}
	}];
	while (done == false) {
		SDL_Delay(1);
	}
	return r;
}

static bool save_cloud_record(CKDatabase *db, CKRecord *r)
{
	__block bool done = false;
	__block bool ret;
	[db saveRecord:r completionHandler:^(CKRecord *savedRecord, NSError *saveError) {
		if (saveError) {
			cloud_error_code = 3;
			ret = false;
			done = true;
		}
		else {
			ret = true;
			done = true;
		}
	}];
	while (done == false) {
		SDL_Delay(1);
	}
	return ret;
}
    
static std::string replace_periods(std::string s)
{
    for (size_t i = 0; i < s.length(); i++) {
        if (s[i] == '.') {
            s[i] = '_';
        }
    }
    return s;
}

Sint64 apple_cloud_date(std::string filename)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	CKDatabase *db = get_cloud_database();
	CKRecord *record = get_cloud_record(db);
	if (record == nil) {
		[pool release];
		return -1;
	}
	[record autorelease];
	filename = filename.substr(0, filename.length()-4) + "_txt";
	NSString *key = [NSString stringWithUTF8String:filename.c_str()];
	NSData *date = record[key];
	cloud_error_code = 0;
	if (date == nil) {
		[pool release];
		return -1;
	}
	std::string date_s((char *)[date bytes], (int)[date length]);
	if (date_s.length() == 0) {
		[pool release];
		return -1;
	}
	Sint64 ret = atoll(date_s.c_str());
	[pool release];
	return ret;
}

bool apple_cloud_delete(std::string filename)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	CKDatabase *db = get_cloud_database();
	CKRecord *record = get_cloud_record(db);
	if (record == nil) {
		[pool release];
		return false;
	}
	[record autorelease];
	filename = replace_periods(filename);
	NSString *key = [NSString stringWithUTF8String:filename.c_str()];
	char bytes[1] = { 0 };
	record[key] = [NSData dataWithBytes:bytes length:0];
	bool ret;
	if (save_cloud_record(db, record) == false) {
		cloud_error_code = 7;
		ret = false;
	}
	else {
		cloud_error_code = 0;
		ret = true;
	}
	[pool release];
	return ret;
}

char *apple_cloud_read(std::string filename, int *sz)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	CKDatabase *db = get_cloud_database();
	CKRecord *record = get_cloud_record(db);
	if (record == nil) {
		[pool release];
		return 0;
	}
	[record autorelease];
	filename = replace_periods(filename);
	NSString *key = [NSString stringWithUTF8String:filename.c_str()];
	NSData *data = record[key];
	if (data == nil) {
		[pool release];
		cloud_error_code = 5;
		return 0;
	}
	char *bytes;
	*sz = (int)[data length];
	bytes = new char[*sz];
	memcpy(bytes, [data bytes], *sz);
	[pool release];
	cloud_error_code = 0;
	return bytes;
}

bool apple_cloud_save(std::string filename, const char *bytes, int sz)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	CKDatabase *db = get_cloud_database();
	CKRecord *record = get_cloud_record(db);
	if (record == nil) {
		[pool release];
		return false;
	}
	[record autorelease];
	filename = replace_periods(filename);
	NSString *key = [NSString stringWithUTF8String:filename.c_str()];
	NSData *data = [NSData dataWithBytes:bytes length:sz];
	record[key] = data;
	bool ret;
	if (save_cloud_record(db, record) == false) {
		cloud_error_code = 7;
		ret = false;
	}
	else {
		cloud_error_code = 0;
		ret = true;
	}
	[pool release];
	return ret;
}

int apple_cloud_get_error_code()
{
	return cloud_error_code;
}

void apple_cloud_synchronise()
{
}
#endif

} // End namespace util

} // End namespace noo
