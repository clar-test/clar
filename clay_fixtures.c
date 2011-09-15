static const char *
fixture_path(const char *base, const char *fixture_name)
{
	static char _path[4096];
	size_t root_len;

	root_len = strlen(base);
	strncpy(_path, base, sizeof(_path));

	if (_path[root_len - 1] != '/')
		_path[root_len++] = '/';

	if (fixture_name[0] == '/')
		fixture_name++;

	strncpy(_path + root_len,
		fixture_name,
		sizeof(_path) - root_len);

	return _path;
}

#ifdef CLAY_FIXTURE_PATH
const char *cl_fixture(const char *fixture_name)
{
	return fixture_path(CLAY_FIXTURE_PATH, fixture_name);
}

const char *cl_fixture_s(const char *fixture_name)
{
	fs_copy(cl_fixture(fixture_name), _clay_path);
	_clay.fixtures_sandboxed = 1;

	return fixture_path(_clay_path, fixture_name);
}

void cl_fixture_cleanup(void)
{
	fs_rm(fixture_path(_clay_path, "*"));
	_clay.fixtures_sandboxed = 0;
}
#endif
