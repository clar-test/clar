#ifdef _WIN32
#	define PLATFORM_SEP '\\'
#else
#	define PLATFORM_SEP '/'
#endif

static char _clay_path[4096];

static int
is_valid_tmp_path(const char *path)
{
	struct stat st;
	return (lstat(path, &st) == 0 &&
		(S_ISDIR(st.st_mode) ||
		S_ISLNK(st.st_mode)) &&
		access(path, W_OK) == 0);
}

static int
find_tmp_path(char *buffer, size_t length)
{
	static const size_t var_count = 4;
	static const char *slashtmp = "/tmp";
	static const char *curdir = ".";
	static const char *env_vars[] = {
		"TMPDIR", "TMP", "TEMP", "USERPROFILE"
 	};

 	size_t i;

#ifdef _WIN32
	if (GetTempPath((DWORD)length, buffer))
		return 1;
#endif

	for (i = 0; i < var_count; ++i) {
		const char *env = getenv(env_vars[i]);
		if (!env)
			continue;

		if (is_valid_tmp_path(env)) {
			strncpy(buffer, env, length);
			return 1;
		}
	}

	/* If the environment doesn't say anything, try to use /tmp */
	if (is_valid_tmp_path(slashtmp)) {
		strncpy(buffer, slashtmp, length);
		return 1;
	}

	/* This system doesn't like us, try to use the current directory */
	if (is_valid_tmp_path(curdir)) {
		strncpy(buffer, curdir, length);
		return 1;
	}


	return 0;
}

static int clean_folder(const char *path)
{
	const char os_cmd[] =
#ifdef _WIN32
		"rd /s /q \"%s\"";
#else
		"rm -rf \"%s\"";
#endif

	char command[4096];
	snprintf(command, sizeof(command), os_cmd, path);
	return system(command);
}

static void clay_unsandbox(void)
{
	clean_folder(_clay_path);
}

static int clay_sandbox(void)
{
	const char path_tail[] = "clay_tmp_XXXXXX";
	size_t len;

	if (!find_tmp_path(_clay_path, sizeof(_clay_path)))
		return 0;

	len = strlen(_clay_path);

	if (_clay_path[len - 1] != PLATFORM_SEP) {
		_clay_path[len++] = PLATFORM_SEP;
	}

	strcpy(_clay_path + len, path_tail);

	if (mktemp(_clay_path) == NULL)
		return 0;

	if (mkdir(_clay_path, 0700) != 0)
		return 0;

	if (chdir(_clay_path) != 0)
		return 0;

	return 1;
}

