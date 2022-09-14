#include <sstream>

#ifdef TVOS
#include <zlib.h>
#endif

#include "shim4/ios.h"
#include "shim4/util.h"

namespace noo {

namespace util {

std::string ios_get_resource_path(std::string file)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSBundle *bundle = [NSBundle mainBundle];
	NSString *s = [NSString stringWithUTF8String:file.c_str()];
	NSString *path = [bundle pathForResource:s ofType:@""];
	std::string ret = [path UTF8String];
	[pool release];
	return ret;
}

std::string ios_get_standard_path(Path_Type type)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

	std::string path;
	NSString *s = 0;

	if (type == DOCUMENTS) {
		s = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0];
	}
	else if (type == APPDATA) {
		s = [NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES) objectAtIndex:0];
	}

	if (s) {
		path = [s UTF8String];
	}
	
	[pool release];

	return path;
}

void ios_mkdir(std::string path)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSString *s = [NSString stringWithUTF8String:path.c_str()];


	NSError *err;

	if ([[NSFileManager defaultManager] createDirectoryAtPath:s withIntermediateDirectories:TRUE attributes:nil error:&err] == NO) {
		NSLog(@"%@", [err localizedDescription]);
	}

	[pool release];
}

bool ios_is_ipad()
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	bool b = [[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad;
	[pool release];
	return b;
}

#ifdef TVOS
char *tvos_read_bytes(std::string filename, int *sz)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

	NSUserDefaults *ud = [NSUserDefaults standardUserDefaults];
	NSString *fn = [NSString stringWithUTF8String:filename.c_str()];
	NSData *data = [ud dataForKey:fn];
	char *ret;
	if (data != nil) {
		*sz = (int)[data length];
		ret = new char[*sz];
		memcpy(ret, (char *)[data bytes], *sz);
	}
	else {
		ret = 0;
	}

	[pool release];

	return ret;
}

bool tvos_read_file(std::string filename, std::string &file)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

	NSUserDefaults *ud = [NSUserDefaults standardUserDefaults];
	NSString *fn = [NSString stringWithUTF8String:filename.c_str()];
	NSData *data = [ud dataForKey:fn];
	bool ret;
	if (data != nil) {
		int sz = (int)[data length];
		unsigned char *bytes = (unsigned char *)[data bytes];
		uLong uncompressed_size = (bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24));
		unsigned char *uncompressed = new unsigned char[uncompressed_size+1];
		if (uncompress((Bytef *)uncompressed, &uncompressed_size, (Bytef *)bytes+4, (uLong)sz-4) != Z_OK) {
			file = "";
			ret = false;
		}
		else {
			uncompressed[uncompressed_size] = 0; // null terminator
			file = std::string((char *)uncompressed);
			ret = true;
		}
		delete[] uncompressed;
	}
	else {
		file = "";
		ret = false;
	}

	[pool release];

	return ret;
}

static std::string time_to_string(Sint64 t)
{
	std::stringstream ss;
	ss << t;
	return ss.str();
}

bool tvos_save_bytes(std::string filename, char *bytes, int sz)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

	NSData *data = [NSData dataWithBytes:bytes length:sz];

	NSUserDefaults *ud = [NSUserDefaults standardUserDefaults];
	NSString *fn = [NSString stringWithUTF8String:filename.c_str()];

	[ud setObject:data forKey:fn];

	std::string time_s = time_to_string(utc_secs());
	
	NSData *date = [NSData dataWithBytes:time_s.c_str() length:time_s.length()];
	NSString *date_fn = [NSString stringWithUTF8String:(filename + ".date").c_str()];

	[ud setObject:date forKey:date_fn];

	[pool release];

	return true;
}

bool tvos_save_file(std::string filename, std::string file)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

	uLong max_compressed_size = compressBound(file.length());
	unsigned char *buf = new unsigned char[max_compressed_size + 4]; // 4 for size
	if (compress((Bytef *)buf+4, &max_compressed_size, (Bytef *)file.c_str(), (uLong)file.length()) != Z_OK) {
		delete[] buf;
		[pool release];
		return false;
	}
	buf[0] = (file.length() & 0xff);
	buf[1] = ((file.length() >> 8) & 0xff);
	buf[2] = ((file.length() >> 16) & 0xff);
	buf[3] = ((file.length() >> 24) & 0xff);

	NSData *data = [NSData dataWithBytes:buf length:(max_compressed_size+4)];

	NSUserDefaults *ud = [NSUserDefaults standardUserDefaults];
	NSString *fn = [NSString stringWithUTF8String:filename.c_str()];

	[ud setObject:data forKey:fn];

	delete[] buf;

	std::string time_s = time_to_string(utc_secs());
	
	NSData *date = [NSData dataWithBytes:time_s.c_str() length:time_s.length()];
	NSString *date_fn = [NSString stringWithUTF8String:(filename + ".date").c_str()];

	[ud setObject:date forKey:date_fn];

	[pool release];

	return true;
}

void tvos_delete_file(std::string filename)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSUserDefaults *ud = [NSUserDefaults standardUserDefaults];
	NSString *fn = [NSString stringWithUTF8String:filename.c_str()];
	[ud removeObjectForKey:fn];
	NSString *fn_date = [NSString stringWithUTF8String:(filename + ".date").c_str()];
	[ud removeObjectForKey:fn_date];
	[pool release];
}

Sint64 tvos_file_date(std::string filename)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

	NSUserDefaults *ud = [NSUserDefaults standardUserDefaults];
	NSString *fn = [NSString stringWithUTF8String:(filename + ".date").c_str()];
	NSData *data = [ud dataForKey:fn];
	Sint64 ret;
	if (data != nil) {
		std::string time_s((char *)[data bytes], (int)[data length]);
		ret = atoll(time_s.c_str());
	}
	else {
		ret = -1;
	}

	[pool release];

	return ret;
}

void tvos_touch(std::string filename)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

	NSUserDefaults *ud = [NSUserDefaults standardUserDefaults];

	NSString *fn = [NSString stringWithUTF8String:filename.c_str()];
	NSData *data = [ud dataForKey:fn];
	if (data == nil) {
		[pool release];
		return;
	}

	std::string time_s = time_to_string(utc_secs());
	
	NSData *date = [NSData dataWithBytes:time_s.c_str() length:time_s.length()];
	NSString *date_fn = [NSString stringWithUTF8String:(filename + ".date").c_str()];

	[ud setObject:date forKey:date_fn];

	[pool release];
}

bool tvos_rename(std::string from, std::string to)
{
	std::string file;
	if (tvos_read_file(from, file) == false) {
		return false;
	}
	tvos_delete_file(from);
	return tvos_save_file(to, file);
}
#endif
} // End namespace util

} // End namespace noo
