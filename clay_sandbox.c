static char _clay_path[4096];

static int
is_valid_tmp_path(const char *path)
{
	STAT_T st;

	if (stat(path, &st) != 0)
		return 0;

	if (!S_ISDIR(st.st_mode))
		return 0;

	return (access(path, W_OK) == 0);
}

static int
find_tmp_path(char *buffer, size_t length)
{
	static const size_t var_count = 4;
	static const char *env_vars[] = {
		"TMPDIR", "TMP", "TEMP", "USERPROFILE"
 	};

#ifdef _WIN32
	if (GetTempPath((DWORD)length, buffer))
		return 0;
#else
 	size_t i;

	for (i = 0; i < var_count; ++i) {
		const char *env = getenv(env_vars[i]);
		if (!env)
			continue;

		if (is_valid_tmp_path(env)) {
			strncpy(buffer, env, length);
			return 0;
		}
	}

	/* If the environment doesn't say anything, try to use /tmp */
	if (is_valid_tmp_path("/tmp")) {
		strncpy(buffer, "/tmp", length);
		return 0;
	}
#endif

	/* This system doesn't like us, try to use the current directory */
	if (is_valid_tmp_path(".")) {
		strncpy(buffer, ".", length);
		return 0;
	}

	return -1;
}

static void clay_unsandbox(void)
{
	if (_clay_path[0] == '\0')
		return;

#ifdef _WIN32
	chdir("..");
#endif

	fs_rm(_clay_path);
}

static int build_sandbox_path(void)
{
	const char path_tail[] = "clay_tmp_XXXXXX";
	size_t len;

	if (find_tmp_path(_clay_path, sizeof(_clay_path)) < 0)
		return -1;

	len = strlen(_clay_path);

#ifdef _WIN32
	{ /* normalize path to POSIX forward slashes */
		size_t i;
		for (i = 0; i < len; ++i) {
			if (_clay_path[i] == '\\')
				_clay_path[i] = '/';
		}
	}
#endif

	if (_clay_path[len - 1] != '/') {
		_clay_path[len++] = '/';
	}

	strncpy(_clay_path + len, path_tail, sizeof(_clay_path) - len);

#ifdef _MSC_VER
	if (_mktemp_s(_clay_path, sizeof(_clay_path)) != 0)
		return -1;

	if (mkdir(_clay_path, 0700) != 0)
		return -1;
#else
	if (mkdtemp(_clay_path) == NULL)
		return -1;
#endif

	return 0;
}

static int clay_sandbox(void)
{
	if (_clay_path[0] == '\0' && build_sandbox_path() < 0)
		return -1;

	if (chdir(_clay_path) != 0)
		return -1;

	return 0;
}

