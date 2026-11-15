# Host assertions: only an observed false property is a semantic failure.
# Tool/setup failures keep exit 2 and never manufacture FAIL/count evidence.
checks=0
error() { echo "ERROR $check_suite: $*" >&2; exit 2; }
fail() { echo "FAIL $check_suite: $*"; echo "INITECH_CHECKS $((checks+1)) failures=1"; exit 1; }
check() {
    check_name=$1; shift
    check_rc=0
    "$@" || check_rc=$?
    case $check_rc in
        0) checks=$((checks+1));;
        1) fail "$check_name";;
        *) error "$check_name: assertion tool exited $check_rc";;
    esac
}
complete() { echo "INITECH_CHECKS $checks failures=0"; }
# A successful parent listing distinguishes a missing product output from an
# unreadable image/tool error. A present file must then be independently read.
read_product() {
    rp_image=$1 rp_path=$2 rp_output=$3
    rp_path=${rp_path#::}; rp_path=${rp_path#/}
    case $rp_path in */*) rp_parent=${rp_path%/*};; *) rp_parent=;; esac
    mdir -a -b -i "$rp_image" "::$rp_parent/" > "$rp_output.names" || error "list $rp_parent"
    check "output $rp_path exists" grep -i -x -F "::/$rp_path" "$rp_output.names"
    mtype -i "$rp_image" "::$rp_path" > "$rp_output" || error "read present file $rp_path"
}
# Keep assertion diagnostics outside the external tool's captured stream.
check_log() {
    cl_name=$1 cl_output=$2; shift 2
    cl_rc=0
    "$@" > "$cl_output" 2>&1 || cl_rc=$?
    case $cl_rc in
        0) checks=$((checks+1));;
        1) cat "$cl_output"; fail "$cl_name";;
        *) cat "$cl_output"; error "$cl_name: tool exited $cl_rc";;
    esac
}
