print("[auto script] project configurator");

projectName = arg[1];
executableFileName = arg[2];

if projectName == nil then
	print("please name the project!\nex: lua project_configurator.lua project_name output_exe_name_no_ext");
	os.exit(1);
end

if executableFileName == nil then
	print("please name the .exe file!\nex: lua project_configurator.lua project_name output_exe_name_no_ext");
	os.exit(1);
end

print("project name: " .. projectName);
print("exe file name: " .. executableFileName .. ".exe");

projConfigs = io.open("project_configs.cmake", "w");
projConfigs:write(string.format("set (PROJECT_NAME \"%s\")\n", projectName));
projConfigs:write(string.format("set (EXECUTABLE_FILE_NAME \"%s\")\n", executableFileName));

print("done generating, enjoy :D");
