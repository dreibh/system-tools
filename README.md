<h1 align="center">
 System-Tools<br />
 <span style="font-size: 75%">Tools for Basic System Management</span><br />
 <a href="https://www.nntb.no/~dreibh/system-tools/">
  <span style="font-size: 75%;">https://www.nntb.no/~dreibh/system-tools</span>
 </a>
</h1>


# 💡 What is System-Tools?

System-Tools is a collection of helpful tools for basic system management of Linux and FreeBSD systems:

- [System-Info](#System-Info) (display banners and system information),
- [System-Maintenance](#system-maintenance) (run basic system maintenance tasks),
- [Print-UTF8](#print-utf8) (print UTF-8 text with options for centering, adjusting, etc.),
- [Text-Block](#text-block) (flexible tool for inserting, replacing or removing text blocks in files or streams),
- [Fingerprint-SSH-Keys](#fingerprint-ssh-keys) (show the machine's SSH public key fingerprints in different formats),
- [Configure-Grub](#configure-grub) (configure options for the GRUB boot loader),
- [Try-Hard](#try-hard) (run a command, with configurable retries on failure),
- [Random-Sleep](#random-sleep) (wait for random time span, with support of fractional seconds).

# 📚 System-Info

System-Info displays basic status information about the system: hostname, uptime, CPU, memory statistics, disk space statistics, SSH public key hashes, and networking information. Furthermore, it can be configured to show one or more banners (for example, a project name). System-Info can be configured to be automatically run when logging in, providing the user an up-to-date overview of the system.

One main purpose of System-Info is to run on login, to particularly show a nice login banner (for example, a project or company logo) and then present the basic system information. For this purpose, System-Info can be configured with banner scripts (by default looked up in /etc/system-info.d or /usr/local/etc/system-info.d), which are processed in alphabetically descending order by file name, like:

* <tt>95-application-logo</tt>,
* <tt>90-project-logo</tt>,
* <tt>60-department-logo</tt>,
* <tt>50-company-logo</tt>,
* <tt>01-example</tt>.

The names of all scripts MUST begin with two decimal numbers. That is, scripts must be named [0-9][0-9]... to be processed by System-Info!

If one of the scripts exits with non-zero exit code, the processing of further banner scripts is stopped. This can be used for preconfiguring a system for example with a department and company logo, where the company logo script terminates further processing. A modified system for a certain project can add a project logo as well. The project logo script may terminate further processing, not showing department and company logos. This may be combined with packaging scripts, for example adding an application logo as part of the application's install package (like adding a script 95-application-logo).

Some examples, using the [banner-helper](src/System-Info/system-info.d/banner-helper) library provided by System-Info:

<table summary="System-Info Banner Examples">
  <tr>
    <td style="text-align: center; vertical-align: center;">
     <a href="src/System-Info/system-info.d/01-example.webp">
     <img alt="" src="src/System-Info/figures/01-example.webp" width="100%" height="100%" />
     </a><br />
     <a href="src/System-Info/system-info.d/01-example">01-example</a><br />
     The default example.
    </td>
    <td style="text-align: center; vertical-align: center;">
     <a href="src/System-Info/system-info.d/09-hostname-example.webp">
     <img alt="" src="src/System-Info/figures/09-hostname-example.webp" width="100%" height="100%" />
     </a><br />
     <a href="src/System-Info/system-info.d/09-hostname-example">09-hostname-example</a><br />
     Dynamically showing the hostname of the machine.
    </td>
    <td style="text-align: center; vertical-align: center;">
     <a href="src/System-Info/system-info.d/10-company-logo-example.webp">
     <img alt="" src="src/System-Info/figures/10-company-logo-example.webp" width="100%" height="100%" />
     </a><br />
     <a href="src/System-Info/system-info.d/10-company-logo-example">10-company-logo-example</a><br />
     A <a href="https://www.simulamet.no/">SimulaMet</a> company branding.
    </td>
  </tr>
  <tr>
    <td style="text-align: center; vertical-align: center;">
     <a href="src/System-Info/system-info.d/10-nornet.webp">
     <img alt="" src="src/System-Info/figures/10-nornet.webp" width="100%" height="100%" />
     </a><br />
     <a href="src/System-Info/system-info.d/10-nornet">10-nornet</a><br />
     A <a href="https://www.nntb.no/">NorNet</a> project branding.
    </td>
    <td style="text-align: center; vertical-align: center;">
     <a href="src/System-Info/system-info.d/18-neat.webp">
     <img alt="" src="src/System-Info/figures/18-neat.webp" width="100%" height="100%" />
     </a><br />
     <a href="src/System-Info/system-info.d/18-neat">18-neat</a><br />
     A <a href="https://neat.nntb.no/">NEAT</a> project branding.
    </td>
    <td style="text-align: center; vertical-align: center;">
     <a href="src/System-Info/system-info.d/30-rsplib.webp">
     <img alt="" src="src/System-Info/figures/30-rsplib.webp" width="100%" height="100%" />
     </a><br />
     <a href="src/System-Info/system-info.d/30-rsplib">30-rsplib</a><br />
     A <a href="https://www.nntb.no/~dreibh/rserpool/">RSPLIB</a> project branding.
    </td>
  </tr>
</table>


# 📚 System-Maintenance

System-Maintenance runs some system maintenance tasks to keep the system clean and up to date. These tasks are:

- Ensuring that all packages are configured,
- Updating the package repositories,
- Removing obsolete kernels,
- Installing all available package updates,
- Auto-removing unused packages,
- Ensuring that Grub (the bootloader) is installed and up-to-date,
- Delete network interface mapping (only on request by option, see below),
- Updating package and file search caches,
- Updating firmware,
- Trimming SSDs and virtual storage.

The typical usage is quite simple, e.g.:

<pre>
sudo System-Maintenance
</pre>


# 📚 Print-UTF8

Print-UTF8 is a simple program to print UTF-8 strings in the console with options for indentation, centering, separator as well as size/length/width information. It can e.g.&nbsp;be utilised for printing System-Info banners, or for displaying error messages like this classic Amiga [Guru Meditation](https://en.wikipedia.org/wiki/Guru_Meditation) example:

<pre>
print-utf8 -n -s "\e[1;31;40;5m█" "▀" "█\e[0m"
echo -e "Software Failure.   Press left mouse button to continue.\nGuru Meditation #00000004.48454C50" | \
print-utf8 -n -C "\e[1;31;40;5m█\e[25m" "\e[5m█\e[0m"
print-utf8 -n -s "\e[1;31;40;5m█" "▄" "█\e[0m"
</pre>

<p style="text-align: center;">
 <a href="guru.webp">
  <img alt="A Guru Meditation Example with Print-UTF8" src="src/Print-UTF8/figures/guru.webp" width="50%" />
 </a>
</p>


# 📚 Text-Block

Text-Block is a flexible tool for inserting, replacing or deleting text blocks in files or streams. The blocks to be modified can be selected by begin/end tags, or line numbers. The static pages of this website are generated by using Text-Block to insert contents like publications and project lists, add new software releases, etc.

For example, the publications list in [index.html](https://www.nntb.no/~dreibh/index.html) is placed between the tags '&lt;!-- BEGIN-OF-PUBLICATIONS --&gt;' and '&lt;!-- END-OF-PUBLICATIONS --&gt;'. Text-Block can be used to manipulate this block:

* To extract the publications list to standard output:

  <pre>
  text-block -i index.html --begin-tag '<!-- BEGIN-OF-PUBLICATIONS -->' --end-tag '<!-- END-OF-PUBLICATIONS -->' --extract
  </pre>

* To delete the publications list and write the page to output.html:

  <pre>
  text-block -i index.html -o output.html --begin-tag '<!-- BEGIN-OF-PUBLICATIONS -->' --end-tag '<!-- END-OF-PUBLICATIONS -->' --delete``
  </pre>

* To replace the publications list by contents from update.block (e.g.&nbsp; generated by [BibTeXConv](https://www.nntb.no/~dreibh/bibtexconv/)), and write the page to output.html:

  <pre>
  text-block -i index.html -o output.html --begin-tag '<!-- BEGIN-OF-PUBLICATIONS -->' --end-tag '<!-- END-OF-PUBLICATIONS -->' --replace update.block
  </pre>


# 📚 Fingerprint-SSH-Keys

Fingerprint-SSH-Keys prints the SSH key fingerprints of the local machine in different formats: SSH hash, DNS SSHFP RR, or Python dictionary. Its typical usage is straightforward:

<pre>
Fingerprint-SSH-Keys
</pre>


# 📚 Configure-Grub

Configure-Grub adjusts a GRUB configuration file by applying a configuration from a template, and merging the existing configurations settings with additional customisations. It can for example be used to set a custom screen resolution (GRUB_GFXMODE option) or startup tune (GRUB_INIT_TUNE option). The [VM Image Builder Scripts](https://github.com/simula/nornet-vmimage-builder-scripts) use Configure-Grub to configure the screen resolution and a boot splash image.


# 📚 Try-Hard

Try-Hard runs a command and retries for a given number of times in case of error, with a delay between the trials.

Example to try a file download up to 3&nbsp;times, with a delay of 60&nbsp;seconds between trials:

<pre>
try-hard 3 60 -- wget -O example.tar.gz https://www.example.net/example.tar.gz
</pre>


# 📚 Random-Sleep

Random-Sleep waits for a random time, selected from a given interval, with support for fractional seconds.

Example to wait between 0.5&nbsp;and 299.5&nbsp;seconds:

<pre>
random-sleep 0.5 299.5 &amp;&amp; echo "Finished waiting!"
</pre>


# 📦 Binary Package Installation

Please use the issue tracker at [https://github.com/dreibh/system-tools/issues](https://github.com/dreibh/system-tools/issues) to report bugs and issues!

## Ubuntu Linux

For ready-to-install Ubuntu Linux packages of System-Tools, see [Launchpad PPA for Thomas Dreibholz](https://launchpad.net/~dreibh/+archive/ubuntu/ppa/+packages?field.name_filter=system-tools&field.status_filter=published&field.series_filter=)!

<pre>
sudo apt-add-repository -sy ppa:dreibh/ppa
sudo apt-get update
sudo apt-get install system-tools
</pre>

## Fedora Linux

For ready-to-install Fedora Linux packages of System-Tools, see [COPR PPA for Thomas Dreibholz](https://copr.fedorainfracloud.org/coprs/dreibh/ppa/package/system-tools/)!

<pre>
sudo dnf copr enable -y dreibh/ppa
sudo dnf install system-tools
</pre>

## FreeBSD

For ready-to-install FreeBSD packages of System-Tools, it is included in the ports collection, see [FreeBSD ports tree index of net/system-tools/](https://cgit.freebsd.org/ports/tree/net/system-tools/)!

<pre>
pkg install system-tools
</pre>

Alternatively, to compile it from the ports sources:

<pre>
cd /usr/ports/net/system-tools
make
make install
</pre>


# 💾 Build from Sources

System-Tools is released under the [GNU General Public Licence&nbsp;(GPL)](https://www.gnu.org/licenses/gpl-3.0.en.html#license-text).

Please use the issue tracker at [https://github.com/dreibh/system-tools/issues](https://github.com/dreibh/system-tools/issues) to report bugs and issues!

## Development Version

The Git repository of the System-Tools sources can be found at [https://github.com/dreibh/system-tools](https://github.com/dreibh/system-tools):

<pre>
git clone https://github.com/dreibh/system-tools
cd system-tools
cmake .
make
</pre>

Contributions:

* Issue tracker: [https://github.com/dreibh/system-tools/issues](https://github.com/dreibh/system-tools/issues).
  Please submit bug reports, issues, questions, etc. in the issue tracker!

* Pull Requests for System-Tools: [https://github.com/dreibh/system-tools/pulls](https://github.com/dreibh/system-tools/pulls).
  Your contributions to System-Tools are always welcome!

* CI build tests of System-Tools: [https://github.com/dreibh/system-tools/actions](https://github.com/dreibh/system-tools/actions).

## Release Versions

See [https://www.nntb.no/~dreibh/system-tools/#current-stable-release](https://www.nntb.no/~dreibh/system-tools/#current-stable-release) for the release packages!


# 🔗 Useful Links

* [NetPerfMeter – A TCP/MPTCP/UDP/SCTP/DCCP Network Performance Meter Tool](https://www.nntb.no/~dreibh/netperfmeter/index.html)
* [HiPerConTracer – High-Performance Connectivity Tracer](https://www.nntb.no/~dreibh/hipercontracer/index.html)
