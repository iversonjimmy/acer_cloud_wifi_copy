#!/usr/bin/python -u

import optparse
import os
import random
import shutil
import subprocess
import sys
import zipfile
import zlib

def writeRandomFile(fn, size):
    f = open(fn, "wb")
    s = "".join(chr(random.randrange(0, 256)) for i in range(size))
    f.write(s)
    f.close()

def readBinaryFileAsHex(fn):
    f = open(fn, "rb")
    s = f.read()
    f.close()
    return "".join(("%02x" % ord(c)) for c in s)

def source_callback(option, opt, value, parser):
    content_list = getattr(parser.values, option.dest, [])
    if content_list == None :
        content_list = []
    content_list.append(value)
    setattr(parser.values, option.dest, content_list)

def setupOptParser():
    p = optparse.OptionParser(add_help_option=False)
    p.add_option("-h", "--help", action="help",
                 help="Show this help message and exit")
    p.add_option("--gvm-sdk-root", "", action="store", dest="gvm_sdk_root",
                 help="Specify the GVM_SDK_ROOT directory",
                 metavar="DIR")
    p.add_option("--gvm-working-dir", "", action="store", dest="gvm_working_dir",
                 default=os.getcwd(),
                 help="Specify the GVM_WORKING_DIR directory (default=CWD)",
                 metavar="DIR")
    p.add_option("--title-id", "", action="store", dest="title_id",
                 default="0001100500000000",
                 help="Specify a 8-byte hex-string title ID",
                 metavar="TITLEID")
    p.add_option("--tmd-data-file", "", action="store", dest="tmd_data_file",
                 help="Specify the TMD Data File",
                 metavar="FILE")
    p.add_option("--tmd-title-version", "", action="store", dest="tmd_title_version",
                 help="Specify the TMD's Title Version",
                 metavar="VERSION")
    p.add_option("--gvm-save-size", "", action="store", dest="gvm_save_size", type="int",
                 help="Specify the Save State Size (GVM only)",
                 metavar="SIZE")
    p.add_option("--tmd-only", "", action="store_true", dest="tmd_only",
                 default=False,
                 help="Generate only a tmd file")                 
    p.add_option("--ticket-id", "", action="store", dest="ticket_id",
                 default="12345678",
                 help="Specify a 4-byte hex-string ticket ID",
                 metavar="TICKETID")
    p.add_option("--link-file", "", action="store", dest="link_file",
                 help="File specifying a list of hard links to create",
                 metavar="FILE")
    p.add_option("--source", "", action="callback",
                 dest="content_list", type="string",
                 callback=source_callback,
                 help="The source file or directory to publish",
                 metavar="FILE")
    p.add_option("--create-zip-file", "", action="store", dest="create_zip_file",
                 help="Create a published zip file package",
                 metavar="FILE")
    p.add_option("--use-7zip", "", action="store_true", dest="use_7zip",
                 default=False,
                 help="Use 7zip for Zip64 large file support")
    return p

def main():

    p = setupOptParser()
    opts, args = p.parse_args()

    ########################################
    temp_dir = "/tmp"

    # Tools
    if opts.gvm_sdk_root == None :
        if "GVM_SDK_ROOT" in os.environ :
            opts.gvm_sdk_root = os.path.abspath(os.environ["GVM_SDK_ROOT"])
        elif "ROOT" in os.environ :
            opts.gvm_sdk_root = os.path.abspath(os.environ["ROOT"]) + "/gvm_sdk"
        else :
            opts.gvm_sdk_root = "/opt/gvm/root"

    opts.gvm_working_dir = os.path.abspath(opts.gvm_working_dir)
    if not os.path.isdir(opts.gvm_working_dir) :
        os.makedirs(opts.gvm_working_dir)
    os.chdir(opts.gvm_working_dir)

    publish_util = opts.gvm_sdk_root + "/sbin/gvmimg"
    tmd_util = opts.gvm_sdk_root + "/sbin/tmd"
    ticket_util = opts.gvm_sdk_root + "/sbin/ticket"
    pki_dir = opts.gvm_sdk_root + "/usr/etc/pki_data/"
    print "PKI DIR: " + pki_dir
    random.seed()

    # Check content list
    content_list = getattr(opts, "content_list", [])
    if content_list == None :
        content_list = []
            
    content_count = len(content_list)
    if content_count == 0 :
        print "Need at least one content directory"
        sys.exit(1)
        
    # Convert to abspath for each content
    for content_index in range(content_count) :
        content_dir = content_list[content_index]
        if os.path.isfile(content_dir) or os.path.isdir(content_dir) :
            content_list[content_index] = os.path.abspath(content_dir)
        else :
            print "Cannot find %s" % content_dir
            sys.exit(1)

    if opts.gvm_save_size != None :
        if opts.gvm_save_size < 1 or opts.gvm_save_size > 128 :
            print "Invalid save state size"
            sys.exit(1)

    # Create title key
    title_key_file = "title.key"
    title_key_file_encrypted = "title.key_encrypted"

    if (os.access(title_key_file, os.R_OK) and
        os.stat(title_key_file).st_size == 16) :
        print "Using existing title key..."
    else :
        print "Creating new title key..."
        writeRandomFile(title_key_file, 16)

    # Check whether we can access pki file
    common_key_file = "%s/common_dpki.aesKey" % pki_dir
    if not os.access(common_key_file, os.R_OK) :
        print "Cannot find common key file"
        sys.exit(1)

    common_key = readBinaryFileAsHex(common_key_file)

    openssl_cmd = ["openssl", "enc", "-aes-128-cbc",
                   "-K", common_key,
                   "-iv", "%s0000000000000000" % opts.title_id,
                   "-nopad",
                   "-in", title_key_file,
                   "-out", title_key_file_encrypted]
        
    print "Encrypting title key..."
    subprocess.check_call(openssl_cmd)

    if not opts.tmd_only :

        # Publish content
        for content_index in range(content_count) :
            content_dir = content_list[content_index]

            gvmimg_cmd = ["%s" % publish_util]
            if os.path.isfile(content_dir) :
                gvmimg_cmd.append("-cf")
            else :
                gvmimg_cmd.append("-c")
            gvmimg_cmd.append("-K")
            gvmimg_cmd.append(title_key_file)
            gvmimg_cmd.append("-T")
            gvmimg_cmd.append("0x%s" % opts.title_id)
            gvmimg_cmd.append("-I")
            gvmimg_cmd.append("%d" % content_index)
            gvmimg_cmd.append("-C")
            gvmimg_cmd.append("0x%08x" % content_index)
            if opts.link_file != None :
                gvmimg_cmd.append("-H")
                gvmimg_cmd.append("%s" % opts.link_file)
            gvmimg_cmd.append("%08x" % content_index)
            gvmimg_cmd.append(content_dir)
            out_f = open("gvmimg.stdout.%d" % content_index, "w")

            print "Publishing content %s ..." % content_dir
            subprocess.check_call(gvmimg_cmd, stdout=out_f)
            out_f.close()

    if opts.tmd_data_file == None and opts.gvm_save_size != None :
        opts.tmd_data_file = "tmd.save.data"
        tmd_data_file = open(opts.tmd_data_file, "wb")
        tmd_data_file.write("".join([chr(0), chr(opts.gvm_save_size)]))
        tmd_data_file.close()

    # Generate TMD file
    tmd_content_list = ",".join((s for s in opts.content_list) if opts.tmd_only
                                else ("%08x.cfm" % i for i in range(content_count)))
    tmd_content_id_list = ",".join("0x%08x" % i for i in range(content_count))
    tmd_content_index_list = ",".join("%d" % i for i in range(content_count))
    print tmd_content_list
    print tmd_content_id_list
    print tmd_content_index_list
    tmd_cmd = [tmd_util, "-P", "d", "-O",
               "-o", "0",     
               "-Z", pki_dir,
               "-K", title_key_file_encrypted,
               "-T", "0x%s" % opts.title_id,
               "-C", "0x80",
               "-n", tmd_content_list,
               "-c", tmd_content_id_list,
               "-i", tmd_content_index_list,
               "-t", ",".join(("1" if opts.tmd_only else "5") for i in range(content_count))]

    if opts.tmd_data_file != None:
        tmd_cmd.append("-N")
        tmd_cmd.append(opts.tmd_data_file)
    if opts.tmd_title_version != None:
        tmd_cmd.append("-m")
        tmd_cmd.append(opts.tmd_title_version)

    print "Generating TMD..."
    subprocess.check_call(tmd_cmd)

    # Verify the TMD file
    tmd_file = "%x.tmd" % int(opts.title_id, 16)
    tmd_cmd = [tmd_util, "-P", "d", "-p", "-Z", pki_dir, "-I", tmd_file]

    print "Verifying TMD..."
    tmd_verify_out_f = open("tmd_verify.stdout.%s" % opts.title_id, "w")
    subprocess.check_call(tmd_cmd, stdout=tmd_verify_out_f)
    tmd_verify_out_f.close()

    # Generate eTicket file
    ticket_cmd = [ticket_util, "-P", "d", "-O", 
                  "-Z", pki_dir,
                  "-C", "0", "-c", "%d" % content_count,
                  "-t", "0x%s" % opts.ticket_id,
                  "-T", "0x%s" % opts.title_id,
                  "-K", title_key_file_encrypted]
    
    print "Generating eTicket..."
    subprocess.check_call(ticket_cmd)

        # Verify eTicket file
    device_key_file = "%s/dev_dpki.eccPvtKey" % pki_dir
    ticket_file="%x.tik" % int(opts.ticket_id, 16)
    ticket_cmd = [ticket_util, "-P", "d", "-p",
                  "-Z", pki_dir,
                  "-I", ticket_file]
    print "Verifying eTicket..."
    tik_verify_out_f = open("%s.tik_verify.stdout" % opts.ticket_id, "w")
    subprocess.check_call(ticket_cmd, stdout=tik_verify_out_f)
    tik_verify_out_f.close()

    if opts.create_zip_file != None:
        if not opts.use_7zip :
            sizelimit = 2*1024*1024*1024L

            zf = zipfile.ZipFile(opts.create_zip_file, "w", zipfile.ZIP_DEFLATED, True)
            try:
                zf.write(tmd_file, "tmd.template")
                zf.write(ticket_file, "eticket.template")
                if opts.tmd_only :
                    for cf in opts.content_list :
                        if os.stat(cf).st_size >= sizelimit :
                            raise RuntimeError("Use 7zip for large file support")
                        zf.write(cf, os.path.basename(cf))
                else :
                    for i in range(content_count) :
                        if os.stat("%08x.blk" % i).st_size >= sizelimit :
                            raise RuntimeError("Use 7zip for large file support")
                        zf.write("%08x.cfm" % i)
                        zf.write("%08x.blk" % i)
            finally:
                zf.close()
        else :
            if opts.tmd_only :
                raise RuntimeError("Doesn't support tmd-only mode with 7zip for now")

            shutil.copyfile(tmd_file, "tmd.template")
            shutil.copyfile(ticket_file, "eticket.template")

            zip_cmd = ["7z", "a", "-y", "-mx0", opts.create_zip_file]
            zip_cmd.append("tmd.template")
            zip_cmd.append("eticket.template")
            for i in range(content_count) :
                zip_cmd.append("%08x.cfm" % i)
                zip_cmd.append("%08x.blk" % i)
            
            print "Running 7 Zip..."
            subprocess.check_call(zip_cmd)

if __name__ == '__main__':
    main()
