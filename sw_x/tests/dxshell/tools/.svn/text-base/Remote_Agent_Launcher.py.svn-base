#!/usr/bin/python
import os
import sys
import fileinput
import subprocess
import time
import datetime
#from pywinauto import application

def doSystemCall(syscall, exitOnFail=False):
    print '%s: START: "%s"' % (str(datetime.datetime.now().time()), syscall)
    # Flush any buffered output before running the system command to prevent its output from being
    # reordered in relation to this Python's print statements.
    sys.stdout.flush()
    rc = os.system(syscall)
    # rc = (actual result << 8) | (terminating signal). Unshift for result only.
    rc = rc >> 8
    print '%s: END  : "%s" RESULT:%d' % (str(datetime.datetime.now().time()), syscall, rc)
    if(exitOnFail and rc != 0) :
        print 'Fatal error! Should abort operation now!'
        sys.exit(rc)
    return rc

class Remote_Agent_Launcher():
    ""
    def __init__(self):
        # set local config file and port file
        self.dxshell_local_work_path = os.getcwd()
        self.dxshell_config_name = "%s/dxshellrc.cfg" % self.dxshell_local_work_path
        self.dxshell_port_txt = "%s/port.txt" % self.dxshell_local_work_path
        self.dxshell_remote_agent_txt = "%s/remote_agent.txt" % self.dxshell_local_work_path
        self.dxshell_port_name = 'port'
        self.remote_work_path = ''

        self.alias = ''
        self.os_name = ''
        self.remote_user = ''
        self.remote_ip = ''
        self.remote_controller_ip = ''

        self.device = ''
        self.ios_sw_path = ''

        self.alias_index = 0
        self.os_index = 1
        self.user_index = 2
        self.ip_index = 3
        self.controller_ip_index = 5

        self.device_index = 1

        self.item_name_index = 0
        self.item_value_index = 1

        self.have_host = False

    def get_host_config(self, dut_alias):
        have_config = False
        have_device = False
        config_file = open(self.dxshell_config_name, 'rb')
        lines = config_file.readlines()
        config_file.close()
        configData = []
        deviceData = []

        for row in lines:
            if len(row) == 0:
                continue
            if '<Config=dxshell_rc>' in row:
                have_config = True
                continue
            elif not row.split():
                continue
            elif row[0] == '#':
                continue
            elif '</Config=dxshell_rc>' in row:
                break
            elif have_config:
                configData.append(row)

        for row in lines:
            if len(row) == 0:
                continue
            if '<Config=device_info>' in row:
                have_device= True
                continue
            elif not row.split():
                continue
            elif row[0] == '#':
                continue
            elif '</Config=device_info>' in row:
                break
            elif have_device:
                deviceData.append(row)


        for row in configData:
            alias_name = self.split_item(row, ',', self.alias_index)
            if alias_name != dut_alias:
                continue

            self.alias = alias_name
            self.remote_work_dir = 'dx_remote_agent_%s' % (self.alias)
            self.os_name = self.split_item(row, ',', self.os_index)
            self.remote_user = self.split_item(row, ',', self.user_index)
            self.remote_ip = self.split_item(row, ',', self.ip_index)

            if self.remote_ip.lower() == "localhost":
                self.remote_ip = "127.0.0.1"

            if self.os_name != 'iOS':
                self.remote_home_path = '/home/%s' % (self.remote_user)
            else:
                self.remote_home_path = '/Users/%s' % (self.remote_user)

            self.remote_work_path = '%s/%s' % (self.remote_home_path, self.remote_work_dir)

            if self.os_name == 'Android' or self.os_name == 'iOS':
                self.remote_controller_ip = self.split_item(row, ',', self.controller_ip_index)

                if self.remote_controller_ip.lower() == "localhost":
                    self.remote_controller_ip = "127.0.0.1"

            self.have_host = True
            break

        for row in deviceData:
            alias_name = self.split_item(row, ',', self.alias_index)
            if alias_name != dut_alias:
                continue
            self.device = self.split_item(row, ',', self.device_index)
            self.device = self.device.replace('\r', '')
            self.device = self.device.replace('\n', '')
            print 'Remote Agent Launcher: device_under_test is "%s"' % (self.device)
            postfix = '-01/builder_ios_test'
            self.ios_sw_path = 'buildslaves/%s%s' % (self.device, postfix)
            break

    def copy_files(self):
        self.stop_processes()
        self.cleanup()
        self.setup()

    def launchApp(self):
        if self.os_name == 'Windows' or self.os_name == 'Android' or self.os_name == 'Linux'  or self.os_name == 'Orbe':
            return
        elif self.os_name == 'WindowsRT':
            doSystemCall('ssh -tt %s@%s "cd %s; python App_Launcher.py %s %s LaunchApp"' % (self.remote_user, self.remote_ip, self.remote_work_path, self.remote_work_path, self.os_name), True)
        elif self.os_name == 'iOS':
            doSystemCall('ssh -tt %s@%s "cd %s; python App_Launcher.py %s %s %s LaunchApp"' % (self.remote_user, self.remote_controller_ip, self.remote_work_path, self.remote_work_path, self.os_name, self.ios_sw_path), True)

    def stopApp(self):
        if self.os_name == 'Windows' or self.os_name == 'Android' or self.os_name == 'Linux' or self.os_name == 'Orbe':
            return
        elif self.os_name == 'WindowsRT':
            doSystemCall('ssh -tt %s@%s "cd %s; python App_Launcher.py %s %s StopApp"' % (self.remote_user, self.remote_ip, self.remote_work_path, self.remote_work_path, self.os_name))
        elif self.os_name == 'iOS':
            doSystemCall('ssh -tt %s@%s "cd %s; python App_Launcher.py %s %s %s StopApp"' % (self.remote_user, self.remote_controller_ip, self.remote_work_path, self.remote_work_path, self.os_name, self.ios_sw_path))

    def restartiOSApp(self):
        if self.os_name == 'Windows' or self.os_name == 'WindowsRT' or self.os_name == 'Linux' or self.os_name == 'Android' or self.os_name == 'Orbe':
            return
        elif self.os_name == 'iOS':
            doSystemCall('ssh -tt %s@%s "export IOS_SW_X_PATH=%s"' % (self.remote_user, self.remote_controller_ip, self.ios_sw_path), True)
            doSystemCall('ssh -tt %s@%s "osascript %s/../%s/sw_x/projects/xcode/PersonalCloud/applescripts/CloseProject.scpt %s/../%s/sw_x/projects/xcode/PersonalCloud dx_remote_agent dx_remote_agent"' % (self.remote_user, self.remote_controller_ip, self.remote_work_path, self.ios_sw_path, self.remote_work_path, self.ios_sw_path), True)
            doSystemCall('ssh -tt %s@%s "security unlock-keychain -p notsosecret ~/Library/Keychains/act-mve.keychain"' % (self.remote_user, self.remote_controller_ip), True)
            doSystemCall('ssh -tt %s@%s "osascript %s/../%s/sw_x/projects/xcode/PersonalCloud/applescripts/RunProject.scpt %s/../%s/sw_x/projects/xcode/PersonalCloud/dx_remote_agent/dx_remote_agent.xcodeproj dx_remote_agent"'  % (self.remote_user, self.remote_controller_ip, self.remote_work_path, self.ios_sw_path, self.remote_work_path, self.ios_sw_path), True)

    def get_port_no_from_remote(self):
        have_port = False
        item_value = ''
        have_remote_agent = False

        # cleanup local side file
        doSystemCall('bash -c "rm -f remote_agent.txt && while [ -e remote_agent.txt ]; do sleep 1; done"')
        doSystemCall('bash -c "rm -f port.txt && while [ -e port.txt ]; do sleep 1; done"')

        #clean port field on config
        if self.os_name == 'Windows' or self.os_name == 'Android' or self.os_name == 'Linux' or self.os_name == 'Orbe':
            self.write_port_no_to_file('')

        if self.os_name == 'WindowsRT' or self.os_name == 'iOS':
            self.write_port_no_to_file(24000)

        for i in range(0, 5):
            if self.os_name == 'iOS':
                doSystemCall('ssh -tt %s@%s "cd %s; python App_Launcher.py %s %s %s UnInstallApp"' % (self.remote_user, self.remote_controller_ip, self.remote_work_path, self.remote_work_path, self.os_name, self.ios_sw_path))
                doSystemCall('ssh -tt %s@%s "cd %s; python App_Launcher.py %s %s %s LaunchApp"' % (self.remote_user, self.remote_controller_ip, self.remote_work_path, self.remote_work_path, self.os_name, self.ios_sw_path))
            elif self.os_name == 'Android':
                doSystemCall('ssh -tt %s@%s "cd %s; nohup /usr/bin/python App_Launcher.py %s %s %s"' % (self.remote_user, self.remote_controller_ip, self.remote_work_path, self.remote_work_path, self.os_name, self.alias))
            elif self.os_name == 'Windows':
                doSystemCall('ssh -tt %s@%s "cd %s; nohup /usr/bin/python App_Launcher.py %s %s %s"' % (self.remote_user, self.remote_ip, self.remote_work_path, self.remote_work_path, self.os_name, self.alias))
            elif self.os_name == 'Linux':
                doSystemCall('ssh -f -tt %s@%s "cd %s; nohup /usr/bin/python App_Launcher.py %s %s %s > %s_remote_agent.log"' % (self.remote_user, self.remote_ip, self.remote_work_path, self.remote_work_path, self.os_name, self.alias, self.alias))
            elif self.os_name == 'Orbe':
                #Need to setup python and nohup on orbe, and have environment variable in /root/setEnv
                doSystemCall('ssh -f root@%s "cd %s; source /root/setEnv; nohup python App_Launcher.py %s %s %s %s > %s_remote_agent.log"' % (self.remote_ip, self.remote_work_path, self.remote_work_path, self.os_name, self.remote_user, self.alias, self.alias))
            elif self.os_name == 'WindowsRT':
                doSystemCall('ssh -tt %s@%s "cd %s; python App_Launcher.py %s %s StopApp"' % (self.remote_user, self.remote_ip, self.remote_work_path, self.remote_work_path, self.os_name))
                doSystemCall('ssh -tt %s@%s "cd %s; python App_Launcher.py %s %s UnInstallApp"' % (self.remote_user, self.remote_ip, self.remote_work_path, self.remote_work_path, self.os_name))
                doSystemCall('ssh -tt %s@%s "cd %s; python App_Launcher.py %s %s InstallApp"' % (self.remote_user, self.remote_ip, self.remote_work_path, self.remote_work_path, self.os_name))
                doSystemCall('ssh -tt %s@%s "cd %s; python App_Launcher.py %s %s LaunchApp"' % (self.remote_user, self.remote_ip, self.remote_work_path, self.remote_work_path, self.os_name))

            if self.os_name == 'WindowsRT' or self.os_name == 'iOS':
                return

            if self.os_name == 'Linux' or self.os_name == 'Orbe':
                doSystemCall('ssh -o "StrictHostKeyChecking no" %s@%s "ps aux | grep dx_remote_agent | grep -v grep > %s/remote_agent.txt"' % (self.remote_user, self.remote_ip, self.remote_work_path))
                doSystemCall('scp -r %s@%s:%s/remote_agent.txt .' % (self.remote_user, self.remote_ip, self.remote_work_path))
            elif self.os_name == 'Windows' :
                doSystemCall('ssh -o "StrictHostKeyChecking no" %s@%s "ps -W aux | grep dx_remote_agent | grep -v grep > %s/remote_agent.txt"' % (self.remote_user, self.remote_ip, self.remote_work_path))
                doSystemCall('scp -r %s@%s:%s/remote_agent.txt .' % (self.remote_user, self.remote_ip, self.remote_work_path))
            elif self.os_name == 'Android':
                # FIXME: This gets process info from the host, not the Android device.
                doSystemCall('ssh -o "StrictHostKeyChecking no" %s@%s "ps -W aux | grep dx_remote_agent | grep -v grep > %s/remote_agent.txt"' % (self.remote_user, self.remote_controller_ip, self.remote_work_path))
                doSystemCall('scp -r %s@%s:%s/remote_agent.txt .' % (self.remote_user, self.remote_controller_ip, self.remote_work_path))

            remote_agent_file = open(self.dxshell_remote_agent_txt, 'rb')
            line = remote_agent_file.readline()
            if line:
                have_remote_agent = True
                break

        if not have_remote_agent:
            print 'Could not find existing remote agent for dut_alias[%s]' % (self.alias)
            sys.exit(102)

        for i in range(0, 5):
            if self.os_name == 'Android':
                doSystemCall('scp -r %s@%s:%s/port_%s.txt ./port.txt' % (self.remote_user, self.remote_controller_ip, self.remote_work_path, self.alias))
            else:
                doSystemCall('scp -r %s@%s:%s/port_%s.txt ./port.txt' % (self.remote_user, self.remote_ip, self.remote_work_path, self.alias))

            if self.os_name == 'Orbe':
                doSystemCall('ssh -o "StrictHostKeyChecking no" root@%s "ulimit -c unlimited && ulimit -c"' % (self.remote_ip))
            elif self.os_name == 'Linux':
                doSystemCall('ssh -o "StrictHostKeyChecking no" %s@%s "ulimit -c unlimited && ulimit -c"' % (self.remote_user, self.remote_ip))

            if not os.path.exists(self.dxshell_port_txt):
                time.sleep(1)
                continue

            port_file = open(self.dxshell_port_txt, 'rb')
            port_lines = port_file.readlines()
            port_file.close()

            for pl in port_lines:
                if not pl:
                    continue

                item_name = self.split_item(pl, '=', self.item_name_index)
                if item_name != self.dxshell_port_name:
                    continue

                item_value = self.split_item(pl, '=', self.item_value_index)
                item_value = self.replace_line(item_value)

                if item_value != '':
                    have_port = True
                    break

            if have_port:
                break

        if not have_port:
            print 'Could not get port number from dut_alias:[%s]' % (self.alias)
            sys.exit(103)

        self.write_port_no_to_file(item_value)

    def write_port_no_to_file(self, port):
        config_list = []
        config_info = ''

        fd = open(self.dxshell_config_name, 'rb')
        lines = fd.readlines()
        fd.close()

        have_config = False
        have_finish = False
        configData = []
        oherData = []

        for row in lines:
            if len(row) == 0:
                continue

            if '<Config=dxshell_rc>' in row:
                have_config = True
                config_list.append(row)
                continue

            if not row.split():
                config_list.append(row)
                continue

            if row[0] == '#':
                config_list.append(row)
                continue

            if '</Config=dxshell_rc>' in row:
                config_list.append(row)
                have_finish = True
                continue

            if have_finish:
                config_list.append(row)
                continue

            if have_config:
                row = self.replace_line(row)
                alias_name = self.split_item(row, ',', self.alias_index)
                if self.alias != alias_name:
                    config_info = row + '\n'
                    config_list.append(config_info)
                    continue

                os_name = self.split_item(row, ',', self.os_index)
                user_name = self.split_item(row, ',', self.user_index)
                ip_addr = self.split_item(row, ',', self.ip_index)
                if os_name == 'Android' or os_name == 'iOS':
                    controller_ip_addr = self.split_item(row, ',', self.controller_ip_index)

                if os_name == 'Windows' or os_name == 'WindowsRT':
                    config_info = '%s,%s,%s,%s,%s\n' % (alias_name, os_name, user_name, ip_addr, port)
                elif os_name == 'Linux' or os_name == 'Orbe':
                    config_info = '%s,%s,%s,%s,%s\n' % (alias_name, os_name, user_name, ip_addr, port)
                elif os_name == 'iOS':
                    config_info = '%s,%s,%s,%s,%s,%s,\n' % (alias_name, os_name, user_name, ip_addr, port, controller_ip_addr)
                else:
                    config_info = '%s,%s,%s,%s,%s,%s,%s\n' % (alias_name, os_name, user_name, ip_addr, '24000', controller_ip_addr, port)

                config_list.append(config_info)

        #print config_list
        fd = open(self.dxshell_config_name, 'w')
        for row in config_list:
            fd.write(row)
        fd.close()

    def replace_line(self, s):
        s = s.replace('\n', '')
        s = s.replace('\r', '')
        return s

    def split_item(self, item, seperate, index):
        return item.split(seperate)[index]

    def stop_processes(self):
        if self.os_name == 'WindowsRT':
            # no action
            return
        elif self.os_name == 'iOS':
            # no action
            return
        elif self.os_name == 'Linux':
            doSystemCall('ssh -o "StrictHostKeyChecking no" %s@%s "ps -ef | grep dx_remote_agent | grep %s | grep -v grep | awk \'{ print \$2 }\' | xargs -tr kill -9"' % (self.remote_user, self.remote_ip, self.alias))
        elif self.os_name == 'Orbe':
            doSystemCall('ssh -o "StrictHostKeyChecking no" %s@%s "ps -ef | grep dx_remote_agent | grep %s | grep -v grep | awk \'{ print \$1 }\' | xargs -tr kill -9"' % (self.remote_user, self.remote_ip, self.alias))
        elif self.os_name == 'Windows':
            doSystemCall('ssh -o "StrictHostKeyChecking no" %s@%s "ps -ef | grep dx_remote_agent_%s | grep -v grep | awk \'{ print $2 }\' | xargs -tr kill -9"' % (self.remote_user, self.remote_ip, self.alias))
        elif self.os_name == 'Android':
            # FIXME: Killing the Android process requires terminating the activity. This command only kills Windows processes.
            doSystemCall('ssh -o "StrictHostKeyChecking no" %s@%s "ps -ef | grep dx_remote_agent_%s | grep -v grep | awk \'{ print $2 }\' | xargs -tr kill -9"' % (self.remote_user, self.remote_controller_ip, self.alias))

        #clean port field on config
        if self.os_name == 'Windows' or self.os_name == 'Android' or self.os_name == 'Linux' or self.os_name == 'Orbe':
            self.write_port_no_to_file('')

        # cleanup local side file
        doSystemCall('bash -c "rm -f remote_agent.txt && while [ -e remote_agent.txt ]; do sleep 1; done"')
        doSystemCall('bash -c "rm -f port.txt && while [ -e port.txt ]; do sleep 1; done"')

        if self.os_name == 'Linux' or self.os_name == 'Orbe':
            doSystemCall('ssh -o "StrictHostKeyChecking no" %s@%s "ps aux | grep dx_remote_agent | grep -v grep > %s/remote_agent.txt"' % (self.remote_user, self.remote_ip, self.remote_work_path))
            doSystemCall('scp -r %s@%s:%s/remote_agent.txt .' % (self.remote_user, self.remote_ip, self.remote_work_path))
        elif self.os_name == 'Windows':
            doSystemCall('ssh -o "StrictHostKeyChecking no" %s@%s "ps -W aux | grep dx_remote_agent | grep -v grep > %s/remote_agent.txt"' % (self.remote_user, self.remote_ip, self.remote_work_path))
            doSystemCall('scp -r %s@%s:%s/remote_agent.txt .' % (self.remote_user, self.remote_ip, self.remote_work_path))
        elif self.os_name == 'Android':
            # FIXME: This collects Windows process info, not Android process info.
            doSystemCall('ssh -o "StrictHostKeyChecking no" %s@%s "ps -W aux | grep dx_remote_agent | grep -v grep > %s/remote_agent.txt"' % (self.remote_user, self.remote_controller_ip, self.remote_work_path))
            doSystemCall('scp -r %s@%s:%s/remote_agent.txt .' % (self.remote_user, self.remote_controller_ip, self.remote_work_path))

    def cleanup(self):
        # cleanup remote side file
        if self.os_name == 'Android' or self.os_name == 'iOS':
            target_ip = self.remote_controller_ip
        else:
            target_ip = self.remote_ip

        if self.os_name == 'Orbe':
            doSystemCall('ssh %s@%s "cd %s && rm -rf %s && while [ -e %s ]; do sleep 1; done"' % (self.remote_user, target_ip, self.remote_home_path, self.remote_work_dir, self.remote_work_dir))
        else:
            doSystemCall('ssh -tt %s@%s "cd %s && rm -rf %s && while [ -e %s ]; do sleep 1; done"' % (self.remote_user, target_ip, self.remote_home_path, self.remote_work_dir, self.remote_work_dir))

    def setup(self):
        if self.os_name == 'Orbe':
            #ssh -tt will sometimes hang the Orbe, so remove -tt option
            doSystemCall('ssh %s@%s "mkdir -p %s; while [ ! -e %s ]; do sleep 1; mkdir -p %s; done"' % (self.remote_user, self.remote_ip, self.remote_work_path, self.remote_work_path, self.remote_work_path), True)
            doSystemCall('scp -r App_Launcher.py %s@%s:%s' % (self.remote_user, self.remote_ip, self.remote_work_path), True)
        elif self.os_name == 'Android' or self.os_name == 'iOS':
            doSystemCall('ssh -tt %s@%s "mkdir -p %s; while [ ! -e %s ]; do sleep 1; mkdir -p %s; done"' % (self.remote_user, self.remote_controller_ip, self.remote_work_path, self.remote_work_path, self.remote_work_path), True)
            doSystemCall('scp -r App_Launcher.py %s@%s:%s' % (self.remote_user, self.remote_controller_ip, self.remote_work_path), True)
        else:
            doSystemCall('ssh -tt %s@%s "mkdir -p %s; while [ ! -e %s ]; do sleep 1; mkdir -p %s; done"' % (self.remote_user, self.remote_ip, self.remote_work_path, self.remote_work_path, self.remote_work_path), True)
            doSystemCall('scp -r App_Launcher.py %s@%s:%s' % (self.remote_user, self.remote_ip, self.remote_work_path), True)

        if self.os_name == 'Windows':
            doSystemCall('scp -r ccd.exe %s@%s:%s' % (self.remote_user, self.remote_ip, self.remote_work_path), True)
            doSystemCall('scp -r dx_remote_agent.exe %s@%s:%s' % (self.remote_user, self.remote_ip, self.remote_work_path), True)
            doSystemCall('scp -r TagEdit.exe %s@%s:%s' % (self.remote_user, self.remote_ip, self.remote_work_path), True)
            doSystemCall('scp -r CCDMonitorService.exe %s@%s:%s' % (self.remote_user, self.remote_ip, self.remote_work_path), True)
        elif self.os_name == 'Linux':
            doSystemCall('scp -r dx_remote_agent %s@%s:%s' % (self.remote_user, self.remote_ip, self.remote_work_path), True)
            doSystemCall('scp -r imltool %s@%s:%s' % (self.remote_user, self.remote_ip, self.remote_work_path), True)
            doSystemCall('scp -r valgrind.supp %s@%s:%s' % (self.remote_user, self.remote_ip, self.remote_work_path), True)
            if (self.alias == 'MD' or self.alias == 'Client') and os.path.exists('client/ccd') :
                # FIXME: Should use environment variable or input param to control this
                # Cloudnode run. Recover the real client CCD (moved in sdk_release_*/Makefile)
                doSystemCall('scp -r client/ccd %s@%s:%s' % (self.remote_user, self.remote_ip, self.remote_work_path), True)
            else :
                doSystemCall('scp -r ccd %s@%s:%s' % (self.remote_user, self.remote_ip, self.remote_work_path), True)
        elif self.os_name == 'Android':
            if sys.platform == 'linux2':
                doSystemCall('scp -r dx_remote_agent %s@%s:%s' % (self.remote_user, self.remote_controller_ip, self.remote_work_path), True)
                doSystemCall('scp -r ccd %s@%s:%s' % (self.remote_user, self.remote_controller_ip, self.remote_work_path), True)
            else:
                doSystemCall('scp -r AdbTool %s@%s:%s' % (self.remote_user, self.remote_controller_ip, self.remote_work_path), True)
                doSystemCall('scp -r dx_remote_agent.exe %s@%s:%s' % (self.remote_user, self.remote_controller_ip, self.remote_work_path), True)
                doSystemCall('scp -r ccd.exe %s@%s:%s' % (self.remote_user, self.remote_controller_ip, self.remote_work_path), True)
                doSystemCall('scp -r TagEdit.exe %s@%s:%s' % (self.remote_user, self.remote_controller_ip, self.remote_work_path), True)
        elif self.os_name == 'WindowsRT':
            doSystemCall('scp -r dx_remote_agent_winrt/dx_remote_agent_1.0.0.0_Win32_Test %s@%s:%s' % (self.remote_user, self.remote_ip, self.remote_work_path), True)
            doSystemCall('scp -r dx_remote_agent_winrt/dx_remote_agent_1.0.0.0_Win32.appxupload %s@%s:%s' % (self.remote_user, self.remote_ip, self.remote_work_path), True)
            doSystemCall('scp -r metro_app_utilities.exe %s@%s:%s' % (self.remote_user, self.remote_ip, self.remote_work_path), True)
        elif self.os_name == 'Orbe':
            doSystemCall('scp -r App_Launcher.sh %s@%s:%s' % (self.remote_user, self.remote_ip, self.remote_work_path), True)
            doSystemCall('scp -r cloudnode/dx_remote_agent_cloudnode %s@%s:%s/dx_remote_agent' % (self.remote_user, self.remote_ip, self.remote_work_path), True)
            doSystemCall('scp -r cloudnode/ccd_cloudnode %s@%s:%s/ccd' % (self.remote_user, self.remote_ip, self.remote_work_path), True)
            doSystemCall('scp -r cloudnode/dxshell_cloudnode %s@%s:%s/dxshell' % (self.remote_user, self.remote_ip, self.remote_work_path), True)
            doSystemCall('scp -r cloudnode/imltool %s@%s:%s/imltool' % (self.remote_user, self.remote_ip, self.remote_work_path), True)

    def collect_ccd_log(self):
        if self.os_name == 'Windows':
            doSystemCall('ssh -tt %s@%s "cd /cygdrive/c/Users/%s/AppData/Local/iGware/SyncAgent_%s; tar -zcf %s_CCDLog.tar.gz logs"' % (self.remote_user, self.remote_ip, self.remote_user, self.alias, self.alias))
            doSystemCall('scp -r %s@%s:/cygdrive/c/Users/%s/AppData/Local/iGware/SyncAgent_%s/%s_CCDLog.tar.gz .' % (self.remote_user, self.remote_ip, self.remote_user, self.alias, self.alias))
        elif self.os_name == 'Linux':
            doSystemCall('ssh -tt %s@%s "cd /home/%s/temp/SyncAgent_%s; tar -zcf %s_CCDLog.tar.gz logs"' % (self.remote_user, self.remote_ip, self.remote_user, self.alias, self.alias))
            doSystemCall('scp -r %s@%s:/home/%s/temp/SyncAgent_%s/%s_CCDLog.tar.gz .' % (self.remote_user, self.remote_ip, self.remote_user, self.alias, self.alias))
        elif self.os_name == 'Orbe':
            #ssh -tt will sometimes hang the Orbe, so remove -tt option
            doSystemCall('ssh %s@%s "cd /home/%s/temp/SyncAgent_%s; tar -zcf %s_CCDLog.tar.gz logs"' % (self.remote_user, self.remote_ip, self.remote_user, self.alias, self.alias))
            doSystemCall('scp -r %s@%s:/home/%s/temp/SyncAgent_%s/%s_CCDLog.tar.gz .' % (self.remote_user, self.remote_ip, self.remote_user, self.alias, self.alias))
        elif self.os_name == 'Android':
            doSystemCall('ssh -tt %s@%s "cd %s; tar -zcf %s_CCDLog.tar.gz cc"' % (self.remote_user, self.remote_controller_ip, self.remote_work_path, self.alias))
            doSystemCall('scp -r %s@%s:%s/%s_CCDLog.tar.gz .' % (self.remote_user, self.remote_controller_ip, self.remote_work_path, self.alias))
        elif self.os_name == 'WindowsRT':
            doSystemCall('ssh -tt %s@%s "cd /cygdrive/c/Users/%s/AppData/Local/Packages/43d863b7-f317-4bf4-a669-9ba42a052c53_bzrwd8f9mzhby/LocalState; tar -zcf %s_CCDLog.tar.gz logs"' % (self.remote_user, self.remote_ip, self.remote_user, self.alias))
            doSystemCall('scp -r %s@%s:/cygdrive/c/Users/%s/AppData/Local/Packages/43d863b7-f317-4bf4-a669-9ba42a052c53_bzrwd8f9mzhby/LocalState/%s_CCDLog.tar.gz .' % (self.remote_user, self.remote_ip, self.remote_user, self.alias))
        elif self.os_name == 'iOS':
            doSystemCall('ssh -tt %s@%s "export IOS_SW_X_PATH=%s"' % (self.remote_user, self.remote_controller_ip, self.ios_sw_path));
            doSystemCall('ssh -tt %s@%s "osascript %s/../%s/sw_x/projects/xcode/PersonalCloud/applescripts/CloseProject.scpt %s/../%s/sw_x/projects/xcode/PersonalCloud dx_remote_agent dx_remote_agent"' % (self.remote_user, self.remote_controller_ip, self.remote_work_path, self.ios_sw_path, self.remote_work_path, self.ios_sw_path))
            doSystemCall('ssh -tt %s@%s "osascript %s/../%s/sw_x/projects/xcode/PersonalCloud/applescripts/BackupAndDeleteApp.scpt Backup dx_remote_agent 1"' % (self.remote_user, self.remote_controller_ip, self.remote_work_path, self.ios_sw_path))
            time.sleep(5) # Is this sleep really necessary?
            doSystemCall('ssh -tt %s@%s "tar -zcf %s_CCDLog.tar.gz *.xcappdata"' % (self.remote_user, self.remote_controller_ip, self.alias))
            doSystemCall('scp -r %s@%s:%s/%s_CCDLog.tar.gz .' % (self.remote_user, self.remote_controller_ip, self.remote_home_path, self.alias))

    def clean_ccd_log(self):
        ssh_cmd = ''
        cd_cmd = ''
        rm_dir = ''
        rm_file = ''

        if self.os_name == 'Windows':
            ssh_cmd = 'ssh -tt ' + self.remote_user + '@' + self.remote_ip
            rm_dir = '/cygdrive/c/Users/' + self.remote_user + '/AppData/Local/iGware/SyncAgent_' + self.alias + '/logs'
            rm_file = '/cygdrive/c/Users/' + self.remote_user + '/AppData/Local/iGware/SyncAgent_' + self.alias + '/*.tar.gz'
        elif self.os_name == 'Linux':
            ssh_cmd = 'ssh -tt ' + self.remote_user + '@' + self.remote_ip
            rm_dir = '/home/' + self.remote_user + '/temp/SyncAgent_' + self.alias + '/logs'
            rm_file = '/home/' + self.remote_user + '/temp/SyncAgent_' + self.alias + '/*.tar.gz'
        elif self.os_name == 'Orbe':
            #ssh -tt will sometimes hang the Orbe, so remove -tt option
            ssh_cmd = 'ssh ' + self.remote_user + '@' + self.remote_ip
            rm_dir = '/home/' + self.remote_user + '/temp/SyncAgent_' + self.alias + '/logs'
            rm_file = '/home/' + self.remote_user + '/temp/SyncAgent_' + self.alias + '/*.tar.gz'
        elif self.os_name == 'Android':
            ssh_cmd = 'ssh -tt ' + self.remote_user + '@' + self.remote_controller_ip
            cd_cmd = 'cd ' + self.remote_work_path + ';'
            rm_dir = 'cc'
            rm_file = '*.tar.gz'
        elif self.os_name == 'WindowsRT':
            ssh_cmd = 'ssh -tt ' + self.remote_user + '@' + self.remote_ip
            rm_dir = '/cygdrive/c/Users/' + self.remote_user + '/AppData/Local/Packages/43d863b7-f317-4bf4-a669-9ba42a052c53_bzrwd8f9mzhby/LocalState/logs'
            rm_file = '/cygdrive/c/Users/' + self.remote_user + '/AppData/Local/Packages/43d863b7-f317-4bf4-a669-9ba42a052c53_bzrwd8f9mzhby/LocalState/*.tar.gz'
        elif self.os_name == 'iOS':
            ssh_cmd = 'ssh -tt ' + self.remote_user + '@' + self.remote_controller_ip
            rm_dir = '*.xcappdata'
            rm_file = '*.tar.gz'

        doSystemCall(ssh_cmd + ' \'' + cd_cmd + 'rm -rf ' + rm_dir + ' && while [ $(ls ' + rm_dir + ' 2> /dev/null | wc -l` != 0 ]; do sleep 1; done\'')
        doSystemCall(ssh_cmd + ' \'' + cd_cmd + 'rm -f ' + rm_file + ' && while [ $(ls ' + rm_file + ' 2> /dev/null | wc -l` != 0 ]; do sleep 1; done\'')

if __name__ == "__main__":
    dut_action = sys.argv[1]
    dut_alias = sys.argv[2]

    ral = Remote_Agent_Launcher()
    ral.get_host_config(dut_alias)

    if not ral.have_host:
        print 'No such dut_alias:[%s] in the dxshellrc.cfg' % (dut_alias)
        sys.exit(101)

    operations = {
        'CopyFiles': ral.copy_files,
        'Init': ral.get_port_no_from_remote,
        'LaunchApp': ral.launchApp,
        'StopApp': ral.stopApp,
        'StopProcess': ral.stop_processes,
        'CollectCCDLog': ral.collect_ccd_log,
        'CleanCCDLog': ral.clean_ccd_log,
        'RestartiOSApp': ral.restartiOSApp
    }
    
    try:
        operations[dut_action]()
    except KeyError:
        print 'Action \'%s\' is not supported. Use a supported action: %s' % (dut_action, operations.keys())
        sys.exit(100)    
