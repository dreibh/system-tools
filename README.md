<h1 align="center">
 System-Tools<br />
 <span style="font-size: 75%">Tools for Basic System Management</span><br />
 <a href="https://www.nntb.no/~dreibh/system-tools/">
  <img alt="" src="src/System-Info/figures/screenshot.webp" width="60%" /><br />
  <span style="font-size: 75%;">https://www.nntb.no/~dreibh/system-tools</span>
 </a>
</h1>


# 💡 What is System-Tools?

System-Tools is a collection of helpful tools for basic system management of Linux and FreeBSD systems:

- [System-Info](#-system-info) (display banners and system information),
- [System-Maintenance](#-system-maintenance) (run basic system maintenance tasks),
- [Reset-Machine-ID](#-reset-machine-id) (reset the machine identity state, particularly for a cloned VM),
- [Print-UTF8](#-print-utf8) (print UTF-8 text with options for centering, adjusting, etc.),
- [Text-Block](#-text-block) (flexible tool for inserting, replacing or removing text blocks in files or streams),
- [Fingerprint-SSH-Keys](#-fingerprint-ssh-keys) (show the machine's SSH public key fingerprints in different formats),
- [Configure-Grub](#-configure-grub) (configure options for the GRUB boot loader),
- [Try-Hard](#-try-hard) (run a command, with configurable retries on failure),
- [Random-Sleep](#-random-sleep) (wait for random time span, with support of fractional seconds).
- [X.509-Tools](#-x.509-tools) (tools for viewing, verifying and testing X.509 certificates),

# 📚 System-Info

System-Info displays basic status information about the system: hostname, uptime, CPU, memory statistics, disk space statistics, SSH public key hashes, and networking information. Furthermore, it can be configured to show one or more banners (for example, a project name). System-Info can be configured to be automatically run when logging in, providing the user an up-to-date overview of the system.

One main purpose of System-Info is to run on login, to particularly show a nice login banner (for example, a project or company logo) and then present the basic system information. For this purpose, System-Info can be configured with banner scripts (by default looked up in /etc/system-info.d or /usr/local/etc/system-info.d), which are processed in alphabetically descending order by file name, like:

* `95-application-logo`,
* `90-project-logo`,
* `60-department-logo`,
* `50-company-logo`,
* `01-example`.

The names of all scripts MUST begin with two decimal numbers. That is, scripts must be named `[0-9][0-9]...` to be processed by System-Info!

If one of the scripts exits with non-zero exit code, the processing of further banner scripts is stopped. This can be used for preconfiguring a system for example with a department and company logo, where the company logo script terminates further processing. A modified system for a certain project can add a project logo as well. The project logo script may terminate further processing, not showing department and company logos. This may be combined with packaging scripts, for example adding an application logo as part of the application's install package (like adding a script `95-application-logo`).

Some examples, using the [`banner-helper`](src/System-Info/system-info.d/banner-helper) library provided by System-Info:

<table summary="System-Info Banner Examples">
  <tr>
    <td style="vertical-align: center;">
     <p align="center">
      <a href="src/System-Info/figures/01-example.webp">
        <img alt="" src="src/System-Info/figures/01-example.webp" width="100%" height="100%" />
      </a><br />
      <tt><a href="src/System-Info/system-info.d/01-example">01-example</a></tt><br />
      The default example.
      </p>
    </td>
    <td style="vertical-align: center;">
     <p align="center">
      <a href="src/System-Info/figures/09-hostname-example.webp">
        <img alt="" src="src/System-Info/figures/09-hostname-example.webp" width="100%" height="100%" />
      </a><br />
      <tt><a href="src/System-Info/system-info.d/09-hostname-example">09-hostname-example</a></tt><br />
      Dynamically showing the hostname of the machine.
     </p>
    </td>
    <td style="vertical-align: center;">
     <p align="center">
      <a href="src/System-Info/figures/10-company-logo-example.webp">
        <img alt="" src="src/System-Info/figures/10-company-logo-example.webp" width="100%" height="100%" />
      </a><br />
      <tt><a href="src/System-Info/system-info.d/10-company-logo-example">10-company-logo-example</a></tt><br />
      A <a href="https://www.simulamet.no/">SimulaMet</a> company branding.
     </p>
    </td>
  </tr>
  <tr>
    <td style="vertical-align: center;">
     <p align="center">
      <a href="src/System-Info/figures/10-nornet.webp">
        <img alt="" src="src/System-Info/figures/10-nornet.webp" width="100%" height="100%" />
      </a><br />
      <tt><a href="src/System-Info/system-info.d/10-nornet">10-nornet</a></tt><br />
      A <a href="https://www.nntb.no/">NorNet</a> project branding.
     </p>
    </td>
    <td style="vertical-align: center;">
     <p align="center">
      <a href="src/System-Info/figures/18-neat.webp">
        <img alt="" src="src/System-Info/figures/18-neat.webp" width="100%" height="100%" />
      </a><br />
      <tt><a href="src/System-Info/system-info.d/18-neat">18-neat</a></tt><br />
      A <a href="https://neat.nntb.no/">NEAT</a> project branding.
     </p>
    </td>
    <td style="vertical-align: center;">
     <p align="center">
      <a href="src/System-Info/figures/30-rsplib.webp">
        <img alt="" src="src/System-Info/figures/30-rsplib.webp" width="100%" height="100%" />
      </a><br />
      <tt><a href="src/System-Info/system-info.d/30-rsplib">30-rsplib</a></tt><br />
      A <a href="https://www.nntb.no/~dreibh/rserpool/">RSPLIB</a> project branding.
     </p>
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

```bash
sudo System-Maintenance
```

The manpage of System-Maintenance contains details and further examples:

```bash
man System-Maintenance
```


# 📚 Reset-Machine-ID

Reset-Machine-ID resets the machine identity state, particularly for a cloned VM, to make it appear as a new machine.  It performs the following tasks:

* Reset `/etc/machine-id`,
* Reset `/var/lib/dbus/machine-id` (symlink to `/etc/machine-id`),
* Change hostname, if a new one is provided.
* Provide hardened settings for SSH client and server.
* Create new SSH key pair.

The changes are made interactively on request only, unless the option `--yes-to-all-i-am-really-sure` is set.

* Reset machine ID, without changing the hostname:

  ```bash
  sudo Reset-Machine-ID
  ```

* Reset machine ID, with changing the hostname to new-hostname.domain.example:

  ```bash
  sudo Reset-Machine-ID --hostname new-hostname.domain.example
  ```

* The manpage of Reset-Machine-ID contains details and further examples:

  ```bash
  man Reset-Machine-ID
  ```


# 📚 Print-UTF8

Print-UTF8 is a simple program to print UTF-8 strings in the console with options for indentation, centering, separator as well as size/length/width information. It can e.g.&nbsp;be utilised for printing System-Info banners, or for displaying error messages like this classic Amiga [Guru Meditation](https://en.wikipedia.org/wiki/Guru_Meditation) example:

```bash
print-utf8 -n -s "\e[1;31;40;5m█" "▀" "█\e[0m"
echo -e "Software Failure.   Press left mouse button to continue.\nGuru Meditation #00000004.48454C50" | \
print-utf8 -n -C "\e[1;31;40;5m█\e[25m" "\e[5m█\e[0m"
print-utf8 -n -s "\e[1;31;40;5m█" "▄" "█\e[0m"
```

<p align="center">
  <a href="src/Print-UTF8/figures/guru.webp">
   <img alt="A Guru Meditation Example with Print-UTF8" src="src/Print-UTF8/figures/guru.webp" width="70%" />
  </a>
</p>

The manpage of Print-UTF8 contains details and various further examples:

```bash
man print-utf8
```


# 📚 Text-Block

Text-Block is a flexible tool for automated editing operations of text blocks in files or streams:

* Copying input,
* Discarding input,
* Enumeration of lines,
* Highlighting blocks,
* Deleting blocks,
* Inserting text before/after marking, as well as
* Replacing blocks.

The blocks to be modified can be selected by begin/end tags, or line numbers. The static pages of this website are generated by using Text-Block to insert contents like publications and project lists, add new software releases, etc.

For example, the publications list in [`index.html`](https://www.nntb.no/~dreibh/index.html) is placed between the tags '&lt;!-- BEGIN-OF-PUBLICATIONS --&gt;' and '&lt;!-- END-OF-PUBLICATIONS --&gt;'. Text-Block can be used to manipulate this block:

* To extract the publications list to standard output:

  ```bash
  text-block -i index.html \
     --begin-tag '<!-- BEGIN-OF-PUBLICATIONS -->' \
     --end-tag '<!-- END-OF-PUBLICATIONS -->' \
     --extract
  ```

* To delete the publications list and write the page to output.html:

  ```bash
  text-block -i index.html -o output.html \
     --begin-tag '<!-- BEGIN-OF-PUBLICATIONS -->' \
     --end-tag '<!-- END-OF-PUBLICATIONS -->' \
     --delete``
  ```

* To replace the publications list by contents from update.block (e.g.&nbsp; generated by [BibTeXConv](https://www.nntb.no/~dreibh/bibtexconv/), and write the page to output.html:

  ```bash
  text-block -i index.html -o output.html \
     --begin-tag '<!-- BEGIN-OF-PUBLICATIONS -->' \
     --end-tag '<!-- END-OF-PUBLICATIONS -->' \
     --replace update.block
  ```

* The manpage of Text-Block contains details and various further examples:

  ```bash
  man text-block
  ```


# 📚 Fingerprint-SSH-Keys

Fingerprint-SSH-Keys prints the SSH key fingerprints of the local machine in different formats: SSH hash, DNS SSHFP RR, or Python dictionary. Its typical usage is straightforward:

```bash
Fingerprint-SSH-Keys
```

The manpage of Fingerprint-SSH-Keys contains details and further examples:

```bash
man Fingerprint-SSH-Keys
```


# 📚 Configure-Grub

Configure-Grub adjusts a GRUB configuration file by applying a configuration from a template, and merging the existing configurations settings with additional customisations. It can for example be used to set a custom screen resolution (GRUB_GFXMODE option) or startup tune (GRUB_INIT_TUNE option). The [Virtual Machine Image Builder and System Installation Scripts](https://www.nntb.no/~dreibh/vmimage-builder-scripts/) use Configure-Grub to configure the screen resolution and a boot splash image.

The manpage of Configure-Grub contains details and further examples:

```bash
man Configure-Grub
```


# 📚 Try-Hard

Try-Hard runs a command and retries for a given number of times in case of error, with a delay between the trials.

Example to try a file download up to 3&nbsp;times, with a delay of 60&nbsp;seconds between trials:

```bash
try-hard 3 60 -- wget -O example.tar.gz \
   https://www.example.net/example.tar.gz
```

The manpage of Try-Hard contains details and further examples:

```bash
man Try-Hard
```


# 📚 Random-Sleep

Random-Sleep waits for a random time, selected from a given interval, with support for fractional seconds.

Example to wait between 0.5&nbsp;and 299.5&nbsp;seconds:

```bash
random-sleep 0.5 299.5 && echo "Finished waiting!"
```

The manpage of Random-Sleep contains details and further examples:

```bash
man Random-Sleep
```


# 📚 X.509-Tools

The X.509-Tools are a set of utilities for viewing, verifying and testing [X.509](https://en.wikipedia.org/wiki/X.509) certificates:

## View-Certificate

View-Certificate displays basic details of a certificate, like subject, common name, etc. Examples:

* Display the Root CA certificate used by [Let's Encrypt](https://letsencrypt.org/), which is usually installed under `/usr/share/ca-certificates/mozilla/ISRG_Root_X1.crt` (Debian/Ubuntu), `/etc/pki/ca-trust/extracted/pem/directory-hash/ISRG_Root_X1.pem` (Fedora), or `/usr/share/certs/trusted/ISRG_Root_X1.pem` (FreeBSD):

  ```bash
  view-certificate /usr/share/ca-certificates/mozilla/ISRG_Root_X1.crt
  ```

* Display the details of the certificate in file `www.nntb.no.crt`:

  ```bash
  view-certificate www.nntb.no.crt
  ```

Also see the manpage of View-Certificate for further details and examples:

  ```bash
  man view-certificate
  ```


## Check-Certificate

Check-Certificate verifies a certificate, by verifying its chain from a given Root CA certificate, and optionally a Certificate Revokation List&nbsp;(CRL) for certificate revokations. The checks are made using [OpenSSL](https://www.openssl.org/). If [GnuTLS](https://gnutls.org/) and/or [Network Security Services&nbsp;(NSS)](https://firefox-source-docs.mozilla.org/security/nss/) are installed as well, the verification is also made by these implementations in addition. This ensures that – in case of success – the certificate and its chain works with all three major X.509 implementations. Examples:

* Verify the server certificate in `My-Server-Certificate.crt` using the Root CA certificate in `My-CA-Certificate.crt`:

  ```bash
  check-certificate My-CA-Certificate.crt My-Server-Certificate.crt
  ```

* The same as above, but in addtion also checking the CRL in `CRL.crl` for certificate revokations:

  ```bash
  check-certificate --crl CRL.crl \
     My-CA-Certificate.crt My-Server-Certificate.crt
  ```

* Verify the certificate in `www.nntb.no.crt` using the [Let's Encrypt](https://letsencrypt.org/) Root CA certificate in `/usr/share/ca-certificates/mozilla/ISRG_Root_X1.crt`:

  ```bash
  check-certificate \
     /usr/share/ca-certificates/mozilla/ISRG_Root_X1.crt \
     www.nntb.no.crt
  ```

Also see the manpage of Check-Certificate for further details and examples:

  ```bash
  man check-certificate
  ```


## Extract-PEM

Extract-PEM extracts an X.509 certificate bundle from a PEM file into separate files for each entry. The output files are named using a given prefix, with extension according to the entry type (i.e.&nbsp;`.crt` for a certificate, `.key` for a key, `.crl` for a CRL). The first entry (usually: the server, client or user certificate) and/or last entry (usually: the Root CA) may be skipped. Examples:

* Extract the PEM file `My-Server-Certificate.crt`, into files `Certificate-<NUMBER>.<EXTENSION>`. The number is starting from&nbsp;1, and provides the position of an entry within the input file:

  ```bash
   extract-pem My-Server-Certificate.crt --output Certificate-
  ```

* Extract the PEM file `My-Server-Certificate.crt`, into files `Intermediate-<NUMBER>.<EXTENSION>`, skipping the first and last entry. That is, only the intermediate certificates are extracted:

  ```bash
   extract-pem My-Server-Certificate.crt \
      --skip-first-entry --skip-last-entry --output Intermediate-
  ```

Also see the manpage of Extract-PEM for further details and examples:

  ```bash
  man extract-pem
  ```


## Test-TLS-Connection

Test-TLS-Connection establishes a Transport Layer Security&nbsp;(TLS) connection to a remote TCP server on a given port number. The X.509 certificate is then verified by [Check-Certificate](#check-certificate). Examples:

* Connect to [www.heise.de]([https://www.heise.de:443) and verify the certificate with the Root CA certificate in `/usr/share/ca-certificates/mozilla/ISRG_Root_X1.crt` (used by [Let's Encrypt](https://letsencrypt.org/)):

  ```bash
  test-tls-connection www.heise.de:443 \
     /usr/share/ca-certificates/mozilla/ISRG_Root_X1.crt
  ```

* Connect to [www.nntb.no]([https://www.nntb.no:443), store the received certificate in `www.nntb.no.crt`, and verify the certificate with the Root CA certificate in `/usr/share/ca-certificates/mozilla/ISRG_Root_X1.crt` (used by [Let's Encrypt](https://letsencrypt.org/)):

  ```bash
  test-tls-connection www.nntb.no:443 \
     /usr/share/ca-certificates/mozilla/ISRG_Root_X1.crt \
     --save-certificate www.nntb.no.crt
  ```

Also see the manpage of Test-TLS-Connection for further details and examples:

  ```bash
  man test-tls-connection
  ```


# 📦 Binary Package Installation

Please use the issue tracker at [https://github.com/dreibh/system-tools/issues](https://github.com/dreibh/system-tools/issues) to report bugs and issues!

## Ubuntu Linux

For ready-to-install Ubuntu Linux packages of System-Tools, see [Launchpad PPA for Thomas Dreibholz](https://launchpad.net/~dreibh/+archive/ubuntu/ppa/+packages?field.name_filter=td-system-tools&field.status_filter=published&field.series_filter=)!

```bash
sudo apt-add-repository -sy ppa:dreibh/ppa
sudo apt-get update
sudo apt-get install td-system-tools
```

## Fedora Linux

For ready-to-install Fedora Linux packages of System-Tools, see [COPR PPA for Thomas Dreibholz](https://copr.fedorainfracloud.org/coprs/dreibh/ppa/package/td-system-tools/)!

```bash
sudo dnf copr enable -y dreibh/ppa
sudo dnf install td-system-tools
```

## FreeBSD

For ready-to-install FreeBSD packages of System-Tools, it is included in the ports collection, see [FreeBSD ports tree index of net/td-system-tools/](https://cgit.freebsd.org/ports/tree/net/td-system-tools/)!

```bash
sudo pkg install system-tools
```

Alternatively, to compile it from the ports sources:

```bash
cd /usr/ports/net/td-system-tools
make
sudo make install
```


# 💾 Build from Sources

System-Tools is released under the [GNU General Public Licence&nbsp;(GPL)](https://www.gnu.org/licenses/gpl-3.0.en.html#license-text).

Please use the issue tracker at [https://github.com/dreibh/system-tools/issues](https://github.com/dreibh/system-tools/issues) to report bugs and issues!

## Development Version

The Git repository of the System-Tools sources can be found at [https://github.com/dreibh/system-tools](https://github.com/dreibh/system-tools):

```bash
git clone https://github.com/dreibh/system-tools
cd system-tools
sudo ci/get-dependencies --install
cmake .
make
```

Note: The script [`ci/get-dependencies`](https://github.com/dreibh/system-tools/blob/master/ci/get-dependencies) automatically  installs the build dependencies under Debian/Ubuntu Linux, Fedora Linux, and FreeBSD. For manual handling of the build dependencies, see the packaging configuration in [`debian/control`](https://github.com/dreibh/system-tools/blob/master/debian/control) (Debian/Ubuntu Linux), [`system-tools.spec`](https://github.com/dreibh/system-tools/blob/master/rpm/td-system-tools.spec) (Fedora Linux), and [`Makefile`](https://github.com/dreibh/system-tools/blob/master/freebsd/td-system-tools/Makefile) FreeBSD.

Contributions:

* Issue tracker: [https://github.com/dreibh/system-tools/issues](https://github.com/dreibh/system-tools/issues).
  Please submit bug reports, issues, questions, etc. in the issue tracker!

* Pull Requests for System-Tools: [https://github.com/dreibh/system-tools/pulls](https://github.com/dreibh/system-tools/pulls).
  Your contributions to System-Tools are always welcome!

* CI build tests of System-Tools: [https://github.com/dreibh/system-tools/actions](https://github.com/dreibh/system-tools/actions).

## Release Versions

See [https://www.nntb.no/~dreibh/system-tools/#current-stable-release](https://www.nntb.no/~dreibh/system-tools/#current-stable-release) for the release packages!


# 🔗 Useful Links

* [Virtual Machine Image Builder and System Installation Scripts](https://www.nntb.no/~dreibh/vmimage-builder-scripts/)
* [NetPerfMeter – A TCP/MPTCP/UDP/SCTP/DCCP Network Performance Meter Tool](https://www.nntb.no/~dreibh/netperfmeter/)
* [HiPerConTracer – High-Performance Connectivity Tracer](https://www.nntb.no/~dreibh/hipercontracer/)
* [SubNetCalc – An IPv4/IPv6 Subnet Calculator](https://www.nntb.no/~dreibh/subnetcalc/)
* [NorNet – A Real-World, Large-Scale Multi-Homing Testbed](https://www.nntb.no/)
* [NEAT – A New, Evolutive API and Transport-Layer Architecture for the Internet](https://neat.nntb.no/)
* [Thomas Dreibholz's Reliable Server Pooling (RSerPool) Page – The RSPLIB Project](https://www.nntb.no/~dreibh/rserpool/)
* [5G-VINNI – 5G Verticals Innovation Infrastructure](https://www.5g-vinni.eu/)
