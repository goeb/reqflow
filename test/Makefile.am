# Enable parallel tests
TESTS = req-trac.test \
req-stat.test \
req-trac-pdf.test \
req-trac-html.test

# include the tests on the distribution
EXTRA_DIST = .
# cleanup tests results before including
dist-hook:
	rm -rf $(distdir)/.git* $(distdir)/*~ $(distdir)/*.out

