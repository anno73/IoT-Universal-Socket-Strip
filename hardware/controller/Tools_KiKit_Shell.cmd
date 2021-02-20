@echo off

:: KiKit for KiCad
::
:: https://github.com/yaqwsx/KiKit
:: https://github.com/yaqwsx/KiKit/blob/master/doc/installation.md#running-kikit-via-docker
::

docker run -it -w /kikit -v "%CD%":/kikit yaqwsx/kikit /bin/bash --login

pause