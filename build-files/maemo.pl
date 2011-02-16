my $machine = `uname -m`;
chomp $machine;
my $mad;
my $asroot;
if ($machine ne "arm") {
    $mad = '/home/uv/apps/mad';
} else {
    $asroot = "fakeroot";
}

my $repo = "https://qgvdial.googlecode.com/svn/trunk";
my $cmd;
my $line;

# Delete any existing version file
if (-f ver.cfg)
{
    unlink(ver.cfg);
}
# Get the latest version file from the repository
$cmd = "svn export $repo/build-files/ver.cfg";
system($cmd);

# Pull out the version from the file
open(QVARFILE, "ver.cfg") or die;
my $qver = <QVARFILE>;
close QVARFILE;

# Get the subversion checkin version
my $svnver = `svn log $repo --limit=1 | grep \"^r\"`;
# Parse out the version number from the output we pulled out
$svnver =~ m/^r(\d+)*/;
$svnver = $1;
# Create the version suffix
$qver = "$qver.$svnver";
my $basedir = "./qgvdial-$qver";

# Delete any previous checkout directories
system("rm -rf qgvdial*");
$cmd = "svn export $repo $basedir";
system($cmd);
system("cp $basedir/icons/Google.png $basedir/src/qgvdial.png");

# Version replacement
$cmd = "cd $basedir ; perl ./build-files/version.pl __QGVDIAL_VERSION__ $qver";
print "$cmd\n";
system($cmd);

# Do everything upto the preparation of the debian directory. Code is still not compiled.
$cmd = "cd $basedir ; $mad qmake && echo y | $mad dh_make --createorig --single -e yuvraaj\@gmail.com -c lgpl && $mad qmake";
system($cmd);

# Add a post install file to add the executable bit after installation on the device
system("mv $basedir/build-files/postinst.maemo $basedir/debian/postinst");
system("mv $basedir/build-files/prerm.maemo $basedir/debian/prerm");
# Fix the control file
system("mv $basedir/build-files/control.maemo $basedir/debian/control");
# Fix the dbus service file name
system("mv $basedir/build-files/qgvdial.Call.service.maemo $basedir/build-files/qgvdial.Call.service");
system("mv $basedir/build-files/qgvdial.Text.service.maemo $basedir/build-files/qgvdial.Text.service");
system("mv $basedir/qgv-tp/data/org.freedesktop.Telepathy.ConnectionManager.qgvtp.service.maemo $basedir/qgv-tp/data/org.freedesktop.Telepathy.ConnectionManager.qgvtp.service");
# Change the name of the desktop file so that it can be directly used in the compilation
system("mv $basedir/build-files/qgvdial.desktop.maemo $basedir/build-files/qgvdial.desktop");

if ($machine eq "arm") {
    $cmd="sed 's/\\/targets\\/FREMANTLE_ARMEL//g' $basedir/Makefile >$basedir/Makefile1 ; mv $basedir/Makefile1 $basedir/Makefile ; sed 's/\\/targets\\/FREMANTLE_ARMEL//g' $basedir/qgv-tp/Makefile >$basedir/qgv-tp/Makefile1 ; mv $basedir/qgv-tp/Makefile1 $basedir/qgv-tp/Makefile ; sed 's/\\/targets\\/FREMANTLE_ARMEL//g' $basedir/qgv-util/Makefile >$basedir/qgv-util/Makefile1 ; mv $basedir/qgv-util/Makefile1 $basedir/qgv-util/Makefile";
    system($cmd);

    $cmd = "cd $basedir && $asroot $mad dpkg-buildpackage -rfakeroot -sa -S";
} else {
    $cmd = "cd $basedir && $asroot $mad dpkg-buildpackage -rfakeroot";
}
# Execute the rest of the build command
system($cmd);

if ($machine ne "arm") {
    $cmd = "dput -f fremantle-upload qgvdial*.changes";
} else {
    $cmd = "dput -f fremantle-extras-builder qgvdial*.changes";
}
system($cmd);

exit();

