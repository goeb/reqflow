
#include <stdio.h>
#include <map>

#include "stringTools.h"

void test(const char *s1, const char *s2)
{

	stringCompare S;
	printf("lessThan(%s, %s) = %d\n", s1, s2, S(s1, s2));
}

main()
{
	std::map<std::string, int, stringCompare> m;

	//test("p3.1.10", "p3.1.10a");
	//test("p3.1.10a", "p3.1.10");

	m["p2.10"] = 0;
	m["p2.1"] = 0;
	m["p3.1.9"] = 0;
	m["p3.1.10"] = 0;
	m["p3.1.10a"] = 0;
	m["p3.2"] = 0;
	m["p2.2"] = 0;
	m["p2.2.1"] = 0;
	m["p2.1.1"] = 0;
	m["p2.1.2"] = 0;

	printf("size: %d\n", m.size());

	std::map<std::string, int, stringCompare>::iterator i;
	for (i = m.begin(); i != m.end(); i++) {
		printf("%s\n", i->first.c_str());
	}
}
