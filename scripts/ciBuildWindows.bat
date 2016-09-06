set startScriptDir=%cd%

set branchName=%1

python -m venv env
CALL env\Scripts\activate.bat

pip install -e git+https://bitbucket.org/osimis/build-helpers.git@0.3.0#egg=buildHelpers
if %errorlevel% neq 0 exit /b %errorlevel%

python buildWindowsOsx.py %branchName%
if %errorlevel% neq 0 exit /b %errorlevel%

CALL deactivate

cd %startScriptDir%