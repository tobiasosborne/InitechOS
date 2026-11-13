* DEPTNAME.PRG -- Department of the current employee
* Initech Systems Corporation.  Sample program for Using InitechBase.
* Use with STAFF.DBF in USE, positioned on an employee.
DO CASE
CASE DEPT = 'SOFT'
  MNAME = 'Software Development'
CASE DEPT = 'CUST'
  MNAME = 'Customer Relations'
CASE DEPT = 'ACCT'
  MNAME = 'Accounting'
CASE DEPT = 'ADMN'
  MNAME = 'Administration'
OTHERWISE
  MNAME = 'Facilities'
ENDCASE
? TRIM(LASTNAME) + ', ' + MNAME
RETURN
