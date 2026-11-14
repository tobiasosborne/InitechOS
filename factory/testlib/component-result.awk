# Independent positive execution receipt, in addition to child exit status.
# Wrappers must identify every child, not borrow a dependency's test count.
BEGIN {
 if(component=="turbo") {want["turbo/seed"]=1;want["turbo/compiler"]=1}
 else if(component=="system") composition=1
 else want[component]=1
}
$2 ~ /^[1-9][0-9]*$/ && $3=="tests," && $4=="0" && $5=="failures" {
 name=$1;sub(/:$/,"",name);if(name in want) seen[name]=1
}
/^system: composition and standalone boundary green$/ {composed=1}
END {
 for(name in want) if(!seen[name]) {print "FAIL receipt: no positive test run for " name;bad=1}
 if(composition && !composed) {print "FAIL receipt: no system boundary checks";bad=1}
 exit bad
}
