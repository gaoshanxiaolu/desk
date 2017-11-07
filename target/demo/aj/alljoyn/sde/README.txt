-----------
Installation:
-----------
0) Follow the setup instructions in the Release Notes for your platform
1) Open terminal application
2) Navigate to this folder in the terminal window
3) Run the 'install.sh' script: ./install.sh --xtensadir $XTENSA_INST

-----------
Use:
-----------
1) Launch Xtensa Xplorer
2) Go to Help -> Help Contents and expand the entry for AllJoyn Development Guide 

-----------
Release notes
-----------
v1.0.0
-----------
* Simplified development environment for AllJoyn development
* Supports the following:
	*Project creation for QCA401x
	*Auto generation of code based on user entered XML files
	*Template/reference AllJoyn files propagated in new projects
	*About feature editors
	*Notification service editors
	*Control Panel service editors
	*Configuration service editors
	*Custom AllJoyn Interface creation/editor with Events/Actions
	*Multi-language AllJoyn support
	*Qonstruct integration into build process

-----------
Known issues
-----------
* Workspace needs manual input for 'Window > Preferences > QCA Platform' if new Project not created.
	1. This is seen when importing the sample without first making a New Project.
	2. Prior to import ensure 'Window > Preferences > QCA Platform' has all entries populated.

* On project rename OR import manual changes required to launch configuration:
	1. Open Debug Configurations
	2. Expand Xtensa On Chip Debug
	3. Select launch configuration for the project
	4. Ensure Name field matches project name
	5. On Main tab, expand Target, select core0:${xt_project}
	6. Expand GDB Commands
	7. Enter correct absolute path for the 'source <PATH>/gen...'
		*where PATH is the absolute path to the project
	8. Click Apply
    
