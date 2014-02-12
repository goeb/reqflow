

test:
	../req trac
	../req stat -s
	../req trac -c req2.conf
	../req trac -c req3.conf || echo "expected error: ok"
	@echo OK
