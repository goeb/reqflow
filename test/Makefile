

test:
	../req trac > trac1.out 2>&1
	diff trac1.out trac1.ref
	../req stat -s > trac2.out 2>&1
	diff trac2.out trac2.ref
	../req trac -c pdf.req > pdf.out 2>&1
	diff pdf.out pdf.ref
	../req trac -c html.req > html.out 2>&1
	diff html.out html.ref
	@echo OK
