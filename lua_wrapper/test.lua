LibTest.TestFunc1(1, 2.3, true, "abcd工了以在", {0,1,2,3});
print(LibTest.MemberStr(pA), LibTest.MemberStr(pB));
LibTest.SetStr(pA, "标题");
LibTest.SetStr(pB, "Message Box Text Info");
print(LibTest.MessageBox(0, LibTest.GetStr(pB), LibTest.GetStr(pA), 3));

LibTest.SetCompoment(pA, {"开头",{1,2,3,4},5.5});
compomentTable = LibTest.MemberCompoment(pA);
print(compomentTable[1]);
for i = 1, #compomentTable[2] do
print(compomentTable[2][i]);
end
print(compomentTable[3]);
print(LibTest.FuncObjAdd(22,33));
print(LibTest.FuncObjSub(22,33));
print(LibTest.StrComp("拼音排序", "排序拼音"));
