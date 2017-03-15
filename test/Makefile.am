# Enable parallel tests
TESTS = req-trac.test \
req-stat.test \
req-stat-stdin.test \
req-trac-pdf.test \
req-trac-html.test \
req-stat-html2.test \
req-stat-docx-rm.test \
req-same-line.test \
req-stat-sort.test \
SPEC-with-table.test \
t_second_doc \
t_end_req

# include the tests on the distribution
EXTRA_DIST = .
# cleanup tests results before including
dist-hook:
	rm -f $(distdir)/.git* $(distdir)/*~ $(distdir)/*.out

clean-local:
	rm -f *.out
