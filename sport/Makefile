.PHONY: test clean
test:
	$(MAKE) -C er4-sport test
	$(MAKE) -C er6g-sport test
	$(MAKE) -C sport-crsf test
	python3 test_preflight.py
clean:
	$(MAKE) -C er4-sport clean
	$(MAKE) -C er6g-sport clean
	$(MAKE) -C sport-crsf clean
