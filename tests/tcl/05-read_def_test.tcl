source [file join [file dirname [info script]] "test_helpers.tcl"]
set current_dir [file dirname [file normalize [info script]]]
set tests_dir [find_parent_dir $current_dir]
set data_dir [file join $tests_dir "data"]

set db [dbDatabase_create]
set lib [odb_read_lef $db $data_dir/gscl45nm.lef]
set chip [odb_read_def $db $data_dir/design.def]
if {$chip == "NULL"} {
    puts "Read DEF Failed"
    exit 1
}
puts $chip
exit 0
