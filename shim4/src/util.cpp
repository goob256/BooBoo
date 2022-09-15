#include "shim4/cpa.h"
#include "shim4/crash.h"
#include "shim4/error.h"
#include "shim4/mt.h"
#include "shim4/shim.h"
#include "shim4/util.h"
#include "shim4/vertex_cache.h"

#ifdef __APPLE__
#include "shim4/apple.h"
#ifdef IOS
#include "shim4/ios.h"
#else
#include "shim4/macosx.h"
#endif
#endif

#ifdef _WIN32
#include <shlobj.h>
#include <dbghelp.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

#ifdef __linux__
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#ifdef ANDROID
#include <jni.h>
#endif

#ifdef STEAMWORKS
#include "shim4/steamworks.h"
#endif

#include "shim4/internal/gfx.h"
#include "shim4/internal/util.h"

using namespace noo;

static bool appdata_dir_set;
static std::string appdata_dir;
static FILE *log_file;

static std::string get_game_name()
{
	if (shim::game_name != "") {
		return shim::game_name;
	}
	return "Nooskewl Shim";
}

#ifdef _WIN32
static void print_string_console(const char *string)
{
	OutputDebugString(string);
	printf("%s", string);
}
#elif defined __APPLE__ && !defined IOS
static void print_string_console(const char *string)
{
	noo::util::macosx_log(string);
}
#elif defined ANDROID
static void print_string_console(const char *string)
{
	JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
	jobject activity = (jobject)SDL_AndroidGetActivity();
	jclass clazz(env->GetObjectClass(activity));

	jstring S = env->NewStringUTF(string);

	jmethodID method_id = env->GetMethodID(clazz, "logString", "(Ljava/lang/String;)V");

	env->CallVoidMethod(activity, method_id, S);

	env->DeleteLocalRef(S);

	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);
}
#else
static void print_string_console(const char *string)
{
	printf("%s", string);
}
#endif

static void print_string(int level, const char *string)
{
	if (level <= 3 && log_file) { // Don't do verbose logging to log file
		fprintf(log_file, "%s", string);
	}
	if (level <= shim::error_level) {
		print_string_console(string);
	}
}

namespace noo {

namespace util {

bool basic_start()
{
	start_crashdumps();
	
	appdata_dir_set = false;
	appdata_dir = "";
	log_file = 0;

	int index;
	if ((index = util::check_args(shim::argc, shim::argv, "+error-level")) > 0) {
		shim::error_level = atoi(shim::argv[index+1]);
	}

	return true;
}

bool static_start()
{
#ifndef IOS
	if (shim::logging) {
#ifdef ANDROID
		std::string log_filename = util::get_standard_path(util::SAVED_GAMES, true) + "/log.txt";
#else
		std::string log_filename = get_appdata_dir() + "/log.txt";
#endif
		log_file = fopen(log_filename.c_str(), "w");
	}
#endif

	return true;
}

void static_end()
{
	internal::close_log_file();

	end_crashdumps();
}

bool start()
{
	srand((uint32_t)time(0));

	return true;
}

void end()
{
}

void mkdir(std::string path)
{
#ifdef _WIN32
	_mkdir(path.c_str());
#elif defined IOS
	ios_mkdir(path);
#else
	::mkdir(path.c_str(), 0700);
#endif
}

void errormsg(const char *fmt, ...)
{
	std::string prefix = get_game_name();
	int len = int(strlen(prefix.c_str()) + 50 + strlen(fmt));
	va_list v;
	char *fmt2 = new char[len];
	char buf[5000];
	if (shim::log_tags) {
		snprintf(fmt2, len, "%s (ERROR): ", prefix.c_str());
	}
	else {
		fmt2[0] = 0;
	}
	strcat(fmt2, fmt);
	va_start(v, fmt);
	vsnprintf(buf, sizeof(buf), fmt2, v);
	print_string(1, buf);
	va_end(v);
	delete[] fmt2;
}

void errormsg(std::string s)
{
	errormsg(s.c_str());
}

void infomsg(const char *fmt, ...)
{
	std::string prefix = get_game_name();
	int len = int(strlen(prefix.c_str()) + 50 + strlen(fmt));
	va_list v;
	char *fmt2 = new char[len];
	char buf[5000];
	if (shim::log_tags) {
		snprintf(fmt2, len, "%s (INFO): ", prefix.c_str());
	}
	else {
		fmt2[0] = 0;
	}
	strcat(fmt2, fmt);
	va_start(v, fmt);
	vsnprintf(buf, sizeof(buf), fmt2, v);
	print_string(2, buf);
	va_end(v);
	delete[] fmt2;
}

void infomsg(std::string s)
{
	infomsg(s.c_str());
}

void debugmsg(const char *fmt, ...)
{
	std::string prefix = get_game_name();
	int len = int(strlen(prefix.c_str()) + 50 + strlen(fmt));
	va_list v;
	char *fmt2 = new char[len];
	char buf[5000];
	if (shim::log_tags) {
		snprintf(fmt2, len, "%s (DEBUG): ", prefix.c_str());
	}
	else {
		fmt2[0] = 0;
	}
	strcat(fmt2, fmt);
	va_start(v, fmt);
	vsnprintf(buf, sizeof(buf), fmt2, v);
	print_string(3, buf);
	va_end(v);
	delete[] fmt2;
}

void debugmsg(std::string s)
{
	debugmsg(s.c_str());
}

void verbosemsg(const char *fmt, ...)
{
	std::string prefix = get_game_name();
	int len = int(strlen(prefix.c_str()) + 50 + strlen(fmt));
	va_list v;
	char *fmt2 = new char[len];
	char buf[5000];
	if (shim::log_tags) {
		snprintf(fmt2, len, "%s (VERBOSE): ", prefix.c_str());
	}
	else {
		fmt2[0] = 0;
	}
	strcat(fmt2, fmt);
	va_start(v, fmt);
	vsnprintf(buf, sizeof(buf), fmt2, v);
	print_string(4, buf);
	va_end(v);
	delete[] fmt2;
}

void verbosemsg(std::string s)
{
	verbosemsg(s.c_str());
}

void printGLerror(const char *fmt, ...)
{
#ifdef DEBUG
	if (true) {
#else
	if (shim::debug) {
#endif
		GLenum error;
		if ((error = glGetError_ptr()) == GL_NO_ERROR) {
			return;
		}
		std::string prefix = get_game_name();
		int len = int(strlen(prefix.c_str()) + 50 + strlen(fmt));
		va_list v;
		char *fmt2 = new char[len];
		char buf[5000];
		if (shim::log_tags) {
			snprintf(fmt2, len, "%s (OPENGL:%d): ", prefix.c_str(), error);
		}
		else {
			fmt2[0] = 0;
		}
		strcat(fmt2, fmt);
		strcat(fmt2, ".\n");
		va_start(v, fmt);
		vsnprintf(buf, sizeof(buf), fmt2, v);
		print_string(3, buf);
		va_end(v);
		delete[] fmt2;
	}
}

int SDL_fgetc(SDL_RWops *file)
{
	unsigned char c;
	if (SDL_RWread(file, &c, 1, 1) == 0) {
		return EOF;
	}
	return c;
}

int SDL_fputc(int c, SDL_RWops *file)
{
	return SDL_RWwrite(file, &c, 1, 1) == 1 ? 1 : EOF;
}

char *SDL_fgets(SDL_RWops *file, char * const buf, size_t max)
{
	size_t c = 0;
	while (c < max) {
		int i = SDL_fgetc(file);
		if (i == -1) {
			break;
		}
		buf[c] = (char)i;
		c++;
		if (i == '\n') {
			break;
		}
	}
	if (c == 0) return 0;
	buf[c] = 0;
	return buf;
}

int SDL_fputs(const char *string, SDL_RWops *file)
{
	size_t len = strlen(string);
	return SDL_RWwrite(file, string, 1, len) < len ? EOF : 0;
}

void SDL_fprintf(SDL_RWops *file, const char *fmt, ...)
{
	char buf[1000];
	va_list v;
	va_start(v, fmt);
	vsnprintf(buf, 1000, fmt, v);
	va_end(v);

	SDL_fputs(buf, file);
}

SDL_RWops *open_file(std::string filename, int *sz, bool data_only)
{
	SDL_RWops *file;
	if (shim::cpa) {
		file = shim::cpa->open(filename, sz, data_only);
	}
	else {
		char *base = SDL_GetBasePath();
		filename = std::string(base) + "data/" + filename;
		SDL_free(base);
		file = SDL_RWFromFile(filename.c_str(), "rb");
		if (file && sz) {
			*sz = (int)SDL_RWsize(file);
			if (data_only) {
				Uint8 *buf = new Uint8[*sz];
				int count = 0;
				const int chunk_size = 32768;
				while (true) {
					int read;
					int to_read = MIN(*sz-count, chunk_size);
					if ((read = (int)SDL_RWread(file, buf+count, 1, to_read)) < to_read) {
						break;
					}
					count += read;
					if (count == *sz) {
						break;
					}
				}
				SDL_RWclose(file);
				return (SDL_RWops *)buf;
			}
		}
	}
	if (file == 0) {
		throw FileNotFoundError(filename);
	}
	return file;
}

void close_file(SDL_RWops *file)
{
	if (shim::cpa) {
		shim::cpa->close(file);
	}
	else {
		SDL_RWclose(file);
	}
}

void free_data(SDL_RWops *file)
{
	if (shim::cpa) {
		shim::cpa->free_data(file);
	}
	// else, do nothing
}

std::string itos(int i)
{
	char buf[20];
	snprintf(buf, 20, "%d", i);
	return std::string(buf);
}

int check_args(int argc, char **argv, std::string arg)
{
	if (argc <= 0 || argv == 0) {
		return -1;
	}

	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], arg.c_str())) {
			return i;
		}
	}
	return -1;
}

bool bool_arg(bool default_value, int argc, char **argv, std::string arg)
{
	if (argc <= 0 || argv == 0) {
		return default_value;
	}

	std::string on = std::string("+") + arg;
	std::string off = std::string("-") + arg;

	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], on.c_str())) {
			default_value = true;
		}
		else if (!strcmp(argv[i], off.c_str())) {
			default_value = false;
		}
	}

	return default_value;
}

std::string string_printf(const char *fmt, ...)
{
	char buf[1000];
	va_list v;
	va_start(v, fmt);
	vsnprintf(buf, 1000, fmt, v);
	va_end(v);

	return buf;
}

std::string uppercase(std::string s)
{
	std::string u;
	for (size_t i = 0; i < s.length(); i++) {
		char str[2];
		str[0] = toupper(s[i]);
		str[1] = 0;
		u += str;
	}
	return u;
}

std::string lowercase(std::string s)
{
	std::string l;
	for (size_t i = 0; i < s.length(); i++) {
		char str[2];
		str[0] = tolower(s[i]);
		str[1] = 0;
		l += str;
	}
	return l;
}

std::string escape_string(std::string s, char c)
{
	std::string ret;

	for (int i = 0; i < (int)s.length(); i++) {
		if (s[i] == c) {
			ret += "\\";
		}
		ret += s.substr(i, 1);
	}

	return ret;
}

std::string unescape_string(std::string s)
{
	std::string ret;

	for (int i = 0; i < (int)s.length(); i++) {
		if (s[i] != '\\') {
			ret += s.substr(i, 1);
		}
	}

	return ret;
}

std::string unescape_string(std::string);

#ifdef _WIN32
List_Directory::List_Directory(std::string filespec) :
	got_first(false),
	done(false)
{
	handle = FindFirstFile(filespec.c_str(), &ffd);
	if (handle == INVALID_HANDLE_VALUE) {
		done = true;
	}
}

List_Directory::~List_Directory()
{
	FindClose(handle);
}

std::string List_Directory::next()
{
	if (done) {
		return "";
	}

	if (got_first == true) {
		if (FindNextFile(handle, &ffd) == 0) {
			done = true;
			return "";
		}
	}
	else {
		got_first = true;
	}

	return ffd.cFileName;
}
#elif !defined ANDROID
List_Directory::List_Directory(std::string filespec) :
	i(0)
{
	gl.gl_pathv = 0;

	int ret = glob(filespec.c_str(), 0, 0, &gl);

	if (ret != 0) {
		i = 0;
	}
}

List_Directory::~List_Directory()
{
	globfree(&gl);
}

std::string List_Directory::next()
{
	if (i >= (int)gl.gl_pathc) {
		i = -1;
	}

	if (i < 0) {
		return "";
	}

	return gl.gl_pathv[i++];
}
#else
List_Directory::List_Directory(std::string filespec)
{
	JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
	jobject activity = (jobject)SDL_AndroidGetActivity();
	jclass clazz(env->GetObjectClass(activity));

	jstring S = env->NewStringUTF(filespec.c_str());

	jmethodID method_id = env->GetMethodID(clazz, "list_dir_start", "(Ljava/lang/String;)V");

	env->CallVoidMethod(activity, method_id, S);

	env->DeleteLocalRef(S);

	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);
}


List_Directory::~List_Directory()
{
}

std::string List_Directory::next()
{
	JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
	jobject activity = (jobject)SDL_AndroidGetActivity();
	jclass clazz(env->GetObjectClass(activity));

	jmethodID method_id = env->GetMethodID(clazz, "list_dir_next", "()Ljava/lang/String;");

	jstring s = (jstring)env->CallObjectMethod(activity, method_id);

	const char *native = env->GetStringUTFChars(s, 0);

	std::string filename = native;

	env->ReleaseStringUTFChars(s, native);

	env->DeleteLocalRef(s);

	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);

	return filename;
}
#endif // _WIN32

std::string load_text(std::string filename)
{
	int size;
	SDL_RWops *file = open_file(filename, &size);

	char *buf = new char[size+1];

	if (SDL_RWread(file, buf, size, 1) != 1) {
		close_file(file);
		throw LoadError(filename);
	}

	buf[size] = 0;

	std::string s = buf;

	delete[] buf;

	close_file(file);

	return s;
}

std::string load_text_from_filesystem(std::string filename)
{
	SDL_RWops *file = SDL_RWFromFile(filename.c_str(), "rb");

	if (file == 0) {
		throw FileNotFoundError(filename);
	}

	int _sz = (int)SDL_RWsize(file);

	char *buf = new char[_sz+1];

	if (SDL_RWread(file, buf, _sz, 1) != 1) {
		throw LoadError(filename);
	}

	SDL_RWclose(file);

	buf[_sz] = 0;

	return buf;
}

char *slurp_file(std::string filename, int *sz)
{
	int _sz;
	SDL_RWops *file = open_file(filename, &_sz);

	char *buf = new char[_sz];

	if (SDL_RWread(file, buf, _sz, 1) != 1) {
		close_file(file);
		throw LoadError(filename);
	}

	close_file(file);

	if (sz) {
		*sz = _sz;
	}

	return buf;
}

char *slurp_file_from_filesystem(std::string filename, int *sz)
{
	SDL_RWops *file = SDL_RWFromFile(filename.c_str(), "rb");

	if (file == 0) {
		throw FileNotFoundError(filename);
	}

	int _sz = (int)SDL_RWsize(file);

	char *buf = new char[_sz];

	if (SDL_RWread(file, buf, _sz, 1) != 1) {
		throw LoadError(filename);
	}

	SDL_RWclose(file);

	if (sz) {
		*sz = _sz;
	}

	return buf;
}

std::string get_standard_path(Path_Type type, bool create)
{
#ifdef _WIN32
	if (type == SAVED_GAMES) {
		std::string userprofile = getenv("USERPROFILE");
		if (userprofile != "") {
			userprofile += "\\Saved Games";
			if (create) {
				mkdir(userprofile);
			}
			return userprofile;
		}
	}

	int i;
	if (type == DOCUMENTS) {
		i = CSIDL_PERSONAL;
	}
	else if (type == APPDATA) {
		i = CSIDL_APPDATA;
	}
	else if (type == HOME) {
		i = CSIDL_PROFILE;
	}
	else {
		return "";
	}

	if (create) {
		i |= CSIDL_FLAG_CREATE;
	}

	char buf[MAX_PATH];

	HRESULT result = SHGetFolderPath(
		gfx::internal::gfx_context.hwnd,
		i,
		NULL,
		0,
		buf
	);

	if (result == S_OK) {
		return std::string(buf);
	}

	return "";
#elif defined __linux__ && !defined ANDROID
	std::string path = getenv("HOME");
	if (create) {
		mkdir(path);
	}
	if (type == DOCUMENTS) {
		path += "/Documents";
	}
	else if (type == APPDATA) {
		path += "/.config";
	}
	if (create) {
		mkdir(path);
	}
	return path;
#elif defined ANDROID
	JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
	jobject activity = (jobject)SDL_AndroidGetActivity();
	jclass clazz(env->GetObjectClass(activity));

	jmethodID method_id;
	
	if (type == SAVED_GAMES) {
		method_id = env->GetMethodID(clazz, "getSDCardDir", "()Ljava/lang/String;");
	}
	else {
		method_id = env->GetMethodID(clazz, "getAppdataDir", "()Ljava/lang/String;");
	}

	jstring s = (jstring)env->CallObjectMethod(activity, method_id);

	const char *native = env->GetStringUTFChars(s, 0);

	std::string path = native;

	if (type == SAVED_GAMES) {
		path += "/" + shim::game_name;
		if (create) {
			mkdir(path.c_str());
		}
	}

	env->ReleaseStringUTFChars(s, native);

	env->DeleteLocalRef(s);

	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);

	return path;
#elif defined IOS
	std::string path = ios_get_standard_path(type);
	if (create) {
		mkdir(path);
	}
	return path;
#else
	std::string path = macosx_get_standard_path(type);
	if (create) {
		mkdir(path);
	}
	return path;
#endif
}

std::string get_appdata_dir()
{
	if (appdata_dir_set) {
		return appdata_dir;
	}

	std::string appdata = get_standard_path(APPDATA, true);
	if (shim::organisation_name != "") {
		appdata += "/" + shim::organisation_name;
		mkdir(appdata);
	}
	appdata += "/" + get_game_name();
	mkdir(appdata);
	return appdata;
}

void set_appdata_dir(std::string appdata_dir, bool create)
{
	if (create) {
		std::string s;
		for (size_t i = 0; i < appdata_dir.length(); i++) {
			char c = appdata_dir[i];
			if (c == '/' || c == '\\') {
				mkdir(s);
			}
			char cs[2];
			cs[0] = c;
			cs[1] = 0;
			s += cs;
		}
	}
	::appdata_dir = appdata_dir;
	appdata_dir_set = true;
}

void open_with_system(std::string filename)
{
#ifdef _WIN32
	if (gfx::internal::gfx_context.fullscreen) {
		ShowWindow(gfx::internal::gfx_context.hwnd, SW_MINIMIZE);
	}
	ShellExecute(0, 0, filename.c_str(), 0, 0 , SW_SHOW);
#elif defined __linux__
	pid_t pid = fork();
	if (pid == 0) {
		system((std::string("xdg-open ") + filename).c_str());
		exit(0);
	}
#elif !defined IOS
	macosx_open_with_system(filename);
#endif
}

void open_url(std::string url)
{
#ifdef ANDROID
	JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
	jobject activity = (jobject)SDL_AndroidGetActivity();
	jclass clazz(env->GetObjectClass(activity));

	jstring S = env->NewStringUTF(url.c_str());

	jmethodID method_id = env->GetMethodID(clazz, "openURL", "(Ljava/lang/String;)V");

	env->CallVoidMethod(activity, method_id, S);

	env->DeleteLocalRef(S);

	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);
#elif defined __linux__
	// FIXME: might work on win/mac too
	open_with_system(url);
#endif
}

#ifdef _WIN32
std::string get_system_language_windows()
{
	LONG l = GetUserDefaultLCID();

	// returns names in Steam format

	if (
		l == 1031 ||
		l == 2055 ||
		l == 3079 ||
		l == 4103 ||
		l == 5127
	) {
		return "german";
	}
	else if (l == 1032) {
		return "greek";
	}
	else if (
		l == 1034 ||
		l == 2058 ||
		l == 3082 ||
		l == 4106 ||
		l == 5130 ||
		l == 6154 ||
		l == 7178 ||
		l == 8202 ||
		l == 9226 ||
		l == 10250 ||
		l == 11274 ||
		l == 12298 ||
		l == 13322 ||
		l == 14346 ||
		l == 15370 ||
		l == 16394 ||
		l == 17418 ||
		l == 18442 ||
		l == 19466 ||
		l == 20490
	) {
		return "spanish";
	}
	else if (
		l == 1036 ||
		l == 2060 ||
		l == 3084 ||
		l == 4108 ||
		l == 5132
	) {
		return "french";
	}
	else if (
		l == 1043 ||
		l == 2067
	) {
		return "dutch";
	}
	else if (l == 1045) {
		return "polish";
	}
	else if (l == 1046) {
		return "brazilian";
	}
	else if (l == 2070) {
		return "portuguese";
	}
	else if (l == 1049) {
		return "russian";
	}
	else if (l == 1042) {
		return "korean";
	}
	else {
		return "english";
	}
}
#endif

#if defined __linux__ && !defined ANDROID
#include <langinfo.h>

std::string get_system_language_linux()
{
	std::string str;
	if (getenv("LANG")) {
		str = getenv("LANG");
	}
	else {
		str = nl_langinfo(_NL_IDENTIFICATION_LANGUAGE);
	}

	std::string l_str = str.substr(0, 5);
	str = str.substr(0, 2);

	// convert to steam style since that was the first one we did
	if (str == "de") {
		str = "german";
	}
	else if (str == "fr") {
		str = "french";
	}
	else if (str == "nl") {
		str = "dutch";
	}
	else if (str == "el") {
		str = "greek";
	}
	else if (str == "it") {
		str = "italian";
	}
	else if (str == "pl") {
		str = "polish";
	}
	else if (str == "pt") {
		if (l_str == "pt_BR") {
			str = "brazilian";
		}
		else {
			str = "portuguese";
		}
	}
	else if (str == "ru") {
		str = "russian";
	}
	else if (str == "es") {
		str = "spanish";
	}
	else if (str == "ko") {
		str = "korean";
	}
	else {
		str = "english";
	}

	return str;
}
#endif

#ifdef ANDROID
std::string get_system_language_android()
{
	JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
	jobject activity = (jobject)SDL_AndroidGetActivity();
	jclass clazz(env->GetObjectClass(activity));

	jmethodID method_id = env->GetMethodID(clazz, "get_android_language", "()Ljava/lang/String;");

	jstring s = (jstring)env->CallObjectMethod(activity, method_id);

	const char *native = env->GetStringUTFChars(s, 0);

	std::string lang = native;

	env->ReleaseStringUTFChars(s, native);

	env->DeleteLocalRef(s);

	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);

	std::string l_str = lang.substr(0, 5);
	std::string str = lang.substr(0, 2);

	// convert to steam style since that was the first one we did
	if (str == "de") {
		str = "german";
	}
	else if (str == "fr") {
		str = "french";
	}
	else if (str == "nl") {
		str = "dutch";
	}
	else if (str == "el") {
		str = "greek";
	}
	else if (str == "it") {
		str = "italian";
	}
	else if (str == "pl") {
		str = "polish";
	}
	else if (str == "pt") {
		if (l_str == "pt-BR") {
			str = "brazilian";
		}
		else {
			str = "portuguese";
		}
	}
	else if (str == "ru") {
		str = "russian";
	}
	else if (str == "es") {
		str = "spanish";
	}
	else if (str == "ko") {
		str = "korean";
	}
	else {
		str = "english";
	}

	return str;
}

bool is_chromebook()
{
	JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
	jobject activity = (jobject)SDL_AndroidGetActivity();
	jclass clazz(env->GetObjectClass(activity));

	jmethodID method_id = env->GetMethodID(clazz, "is_chromebook", "()Z");

	bool result = (bool)env->CallBooleanMethod(activity, method_id);

	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);

	return result;
}
#endif

std::string get_system_language()
{
#ifdef STEAMWORKS
	if (shim::steam_init_failed == false) {
		return get_steam_language();
	}
#endif
#ifdef _WIN32
	return get_system_language_windows();
#elif defined __linux__ && !defined ANDROID
	return get_system_language_linux();
#elif defined ANDROID
	return get_system_language_android();
#else
	return apple_get_system_language();
#endif
}

std::string &ltrim(std::string &s)
{
	int i = 0;
	while (i < (int)s.length() && isspace(s[i])) {
		i++;
	}
	if (i >= (int)s.length()) {
		s = "";
	}
	else {
		s = s.substr(i);
	}
	return s;
}

std::string &rtrim(std::string &s)
{
	int i = (int)s.length() - 1;
	while (i >= 0 && isspace(s[i])) {
		i--;
	}
	if (i < 0) {
		s = "";
	}
	else {
		s = s.substr(0, i+1);
	}
	return s;
}

std::string &trim(std::string &s)
{
	return ltrim(rtrim(s));
}

Uint64 file_date(std::string filename)
{
#ifdef TVOS
	return tvos_file_date(filename);
#else
	struct stat s;
	if (stat(filename.c_str(), &s) == 0) {
		return s.st_ctime;
	}
	else {
		return -1;
	}
#endif
}

#ifndef _WIN32 // FIXME: implement for Windows
time_t utc_secs()
{
	struct timeval tv;
	gettimeofday(&tv, 0);
	return tv.tv_sec;
}
#endif

namespace internal {

#ifdef _WIN32
int c99_vsnprintf(char* str, int size, const char* format, va_list ap)
{
    int count = -1;

    if (size != 0)
        count = _vsnprintf_s(str, size, _TRUNCATE, format, ap);
    if (count == -1)
        count = _vscprintf(format, ap);

    return count;
}

int c99_snprintf(char* str, int size, const char* format, ...)
{
    int count;
    va_list ap;

    va_start(ap, format);
    count = c99_vsnprintf(str, size, format, ap);
    va_end(ap);

    return count;
}
#endif // _WIN32

void close_log_file()
{
	if (log_file) {
		fclose(log_file);
		log_file = 0;
	}
}

void flush_log_file()
{
	fflush(log_file);
}

} // End namespace internal

} // End namespace util

} // End namespace noo
