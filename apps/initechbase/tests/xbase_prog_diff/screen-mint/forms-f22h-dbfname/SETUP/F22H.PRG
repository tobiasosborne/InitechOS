SET ALTERNATE TO RESULT
SET ALTERNATE ON
USE form
? "[" + DBF() + "]"
USE FORM.DBF
? "[" + DBF() + "]"
USE C:FORM
? "[" + DBF() + "]"
USE \SUB\FORM
? "[" + DBF() + "]"
USE SUB\FORM
? "[" + DBF() + "]"
USE FORM ALIAS XYZ
? "[" + DBF() + "]"
SELECT 2
USE FORM2
? "[" + DBF() + "]"
SELECT 1
? "[" + DBF() + "]"
CLOSE DATABASES
? "[" + DBF() + "]"
SET ALTERNATE OFF
SET ALTERNATE TO
RETURN
