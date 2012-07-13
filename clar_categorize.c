#define CLAR_CATEGORY_DEFAULT "default"

typedef struct {
	const char **names;
	int count;
	int alloc;
} clar_category_list;

static int _clar_categorize_suite_is_enabled = 0;
static int _clar_categorize_remember_categories = 0;
static clar_category_list _clar_categorize_enabled;
static clar_category_list _clar_categorize_all;

static int clar_category_in_list(clar_category_list *list, const char *cat)
{
	int i;
	for (i = 0; i < list->count; ++i)
		if (strcasecmp(cat, list->names[i]) == 0)
			return 1;
	return 0;
}

static void clar_category_add_to_list(clar_category_list *list, const char *cat)
{
	if (!clar_category_in_list(list, cat)) {
		if (list->count >= list->alloc) {
			list->alloc += 10;
			list->names =
				realloc(list->names, list->alloc * sizeof(const char *));
		}

		list->names[list->count++] = cat;
	}
}

static void clar_category_enable(const char *category)
{
	clar_category_add_to_list(&_clar_categorize_enabled, category);
}

static void clar_category_setup_print(void)
{
	_clar_categorize_remember_categories = 1;
}

static int clar_category_cmp(const void *a, const void *b)
{
	return strcmp(a,b);
}

static void clar_category_print(const char *prefix)
{
	clar_category_list *list = &_clar_categorize_all;
	int i;

	cl_in_category(CLAR_CATEGORY_DEFAULT);

	qsort(list->names, list->count, sizeof(const char *), clar_category_cmp);

	for (i = 0; i < list->count; ++i)
		printf("%s%s\n", prefix, list->names[i]);
}

static int clar_category_is_suite_enabled(const struct clar_suite *suite)
{
	_clar_categorize_suite_is_enabled = 0;

	if (!_clar_categorize_enabled.count)
		clar_category_enable(CLAR_CATEGORY_DEFAULT);

	if (suite->categorize.ptr)
		suite->categorize.ptr();
	else
		cl_in_category(CLAR_CATEGORY_DEFAULT);

	return (_clar_categorize_suite_is_enabled != 0);
}

void cl_in_category(const char *category)
{
	if (_clar_categorize_remember_categories) {
		clar_category_add_to_list(&_clar_categorize_all, category);
		return;
	}

	if (clar_category_in_list(&_clar_categorize_enabled, category))
		_clar_categorize_suite_is_enabled = 1;
}
