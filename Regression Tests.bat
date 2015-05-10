bin\lua5.1_gfg_i64 rdxc\rdxc.lua rdxsrc/Core.rdxproj
bin\lua5.1_gfg_i64 rdxc\rdxc.lua rdxsrc/Apps/Common.rdxproj
bin\lua5.1_gfg_i64 rdxc\rdxc.lua -nocache -dumpasm=RegressionTest.rdxasm -traceerrors rdxsrc/Apps/RegressionTest.rdxproj

rem bin\rdxbin.exe -plugin rdxcorelib -xil "src/pccm/regressiontest_pccm.xil" Apps.RegressionTest.RegressionTest
rem bin\lua5.1_gfg_i64 "rdxc\rdxncc.lua" "src/pccm/" "regressiontest_pccm" "src/pccm/regressiontest_pccm.xil" "RDX::PCCM::RegressionTest" "../"
rem bin\rdx.exe -plugin rdxcorelib Apps.RegressionTest.RegressionTest arg0 arg1 arg2

@pause
