file (MAKE_DIRECTORY "${WORK_DIR}")
string (RANDOM LENGTH 12 run_id)
set (profile "${WORK_DIR}/${run_id}")
file (MAKE_DIRECTORY "${profile}/Library/Preferences" "${profile}/Library/Application Support"
  "${profile}/Documents" "${profile}/config" "${profile}/data" "${profile}/cache")
foreach (phase IN ITEMS stage verify)
  execute_process (
    COMMAND "${CMAKE_COMMAND}" -E env
      "HOME=${profile}" "CFFIXED_USER_HOME=${profile}"
      "XDG_CONFIG_HOME=${profile}/config" "XDG_DATA_HOME=${profile}/data" "XDG_CACHE_HOME=${profile}/cache"
      "QT_QPA_PLATFORM=offscreen" "WSJTX_FILE_IMPORT_TEST_ROOT=${profile}"
      "WSJTX_FILE_IMPORT_TEST_CASE=${CASE}" "WSJTX_FILE_IMPORT_TEST_PHASE=${phase}"
      "WSJT_QMAP_SHARED_MEMORY_KEY=mem_qmap-import-${run_id}"
      "${APPLICATION}" --startup-smoke-test --rig-name "CTEST-IMPORT-${run_id}" --language en
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 25)
  file (WRITE "${profile}/${phase}.log" "${output}\n${errors}")
  if (NOT result STREQUAL "0" OR NOT errors MATCHES "File import smoke: ${phase} passed")
    message (FATAL_ERROR "File import ${CASE} ${phase} failed (${result}). See ${profile}/${phase}.log\n${errors}")
  endif ()
endforeach ()
