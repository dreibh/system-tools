class TdSystemTools < Formula
  desc "Tools for basic system management and maintenance"
  homepage "https://www.nntb.no/~dreibh/system-tools/"
  url "https://www.nntb.no/~dreibh/system-tools/download/td-system-tools-2.7.14.tar.xz"
  sha256 "160345f8415ea0c8a32b0db338c17bd924096d201e8b6acbf00247666dd16443"
  license "GPL-3.0-or-later"

  # Options OFF by default (matching OPTIONS_DEFAULT exclusions in FreeBSD)
  option "with-configure-grub", "Include Configure-GRUB"
  option "with-gimp-scripts", "Include GIMP-Scripts"
  option "with-gimp-scripts-examples", "Include GIMP-Scripts example files"

  # Options ON by default (matching OPTIONS_DEFAULT in FreeBSD)
  option "without-fingerprint-ssh-keys", "Build without Fingerprint-SSH-Keys"
  option "without-get-system-info", "Build without get-system-info"
  option "without-nls", "Build without Internationalization (i18n) support"
  option "without-print-utf8", "Build without Print-UTF8"
  option "without-random-sleep", "Build without Random-Sleep"
  option "without-reset-machine-id", "Build without Reset-Machine-ID"
  option "without-system-info", "Build without System-Info"
  option "without-system-info-default-banner", "Build without System-Info Default Banner"
  option "without-system-info-examples", "Build without System-Info Examples"
  option "without-system-info-in-profiles", "Build without System-Info in Profiles"
  option "without-system-maintenance", "Build without System-Maintenance"
  option "without-text-block", "Build without Text-Block"
  option "without-try-hard", "Build without Try-Hard"
  option "without-unix-timestamp-tools", "Build without Unix-Timestamp Tools"
  option "without-x509-tools", "Build without X.509 Tools"

  # Mandatory build-time dependencies
  depends_on "cmake" => :build
  depends_on "ninja" => :build
  depends_on "pkg-config" => :build

  # Alphabetically sorted feature runtime dependencies
  depends_on "bash"
  depends_on "figlet" if build.with?("system-info") ||
                         build.with?("system-info-default-banner") ||
                         build.with?("system-info-examples") ||
                         build.with?("system-info-in-profiles")
  depends_on "fontconfig" if build.with?("gimp-scripts") || build.with?("gimp-scripts-examples")
  depends_on "gettext" if build.with?("nls")
  depends_on "gnu-getopt"
  depends_on "graphicsmagick" if build.with?("gimp-scripts") || build.with?("gimp-scripts-examples")
  depends_on "mbuffer" if build.with?("system-info") ||
                          build.with?("system-info-default-banner") ||
                          build.with?("system-info-examples") ||
                          build.with?("system-info-in-profiles") ||
                          build.with?("x509-tools")
  depends_on "openssl@3" if build.with?("x509-tools")
  depends_on "python@3.12" if build.with?("x509-tools")

  def install
    gimp_scripts = build.with?("gimp-scripts") || build.with?("gimp-scripts-examples")

    system_info = build.with?("system-info") ||
                  build.with?("system-info-default-banner") ||
                  build.with?("system-info-examples") ||
                  build.with?("system-info-in-profiles")

    get_system_info = build.with?("get-system-info") || system_info
    random_sleep = build.with?("random-sleep") || build.with?("try-hard")
    x509_tools = build.with?("x509-tools")
    print_utf8 = build.with?("print-utf8") || system_info || x509_tools
    text_block = build.with?("text-block") || x509_tools
    unix_timestamp_tools = build.with?("unix-timestamp-tools") || x509_tools

    args = std_cmake_args + %W[
      -GNinja
      -DCMAKE_MACOSX_RPATH=ON
      -DCMAKE_BUILD_WITH_INSTALL_RPATH=ON
      -DCMAKE_INSTALL_RPATH_USE_LINK_PATH=ON
      -DCMAKE_INSTALL_RPATH=#{rpath}
      -DWITH_CONFIGURE_GRUB=#{build.with?("configure-grub") ? "ON" : "OFF"}
      -DWITH_FINGERPRINT_SSH_KEYS=#{build.with?("fingerprint-ssh-keys") ? "ON" : "OFF"}
      -DWITH_GET_SYSTEM_INFO=#{get_system_info ? "ON" : "OFF"}
      -DWITH_GIMP_SCRIPTS=#{gimp_scripts ? "ON" : "OFF"}
      -DWITH_GIMP_SCRIPTS_EXAMPLES=#{build.with?("gimp-scripts-examples") ? "ON" : "OFF"}
      -DWITH_I18N=#{build.with?("nls") ? "ON" : "OFF"}
      -DWITH_PRINT_UTF8=#{print_utf8 ? "ON" : "OFF"}
      -DWITH_RANDOM_SLEEP=#{random_sleep ? "ON" : "OFF"}
      -DWITH_RESET_MACHINE_ID=#{build.with?("reset-machine-id") ? "ON" : "OFF"}
      -DWITH_SYSTEM_INFO=#{system_info ? "ON" : "OFF"}
      -DWITH_SYSTEM_INFO_DEFAULT_BANNER=#{build.with?("system-info-default-banner") ? "ON" : "OFF"}
      -DWITH_SYSTEM_INFO_EXAMPLES=#{build.with?("system-info-examples") ? "ON" : "OFF"}
      -DWITH_SYSTEM_INFO_IN_PROFILES=#{build.with?("system-info-in-profiles") ? "ON" : "OFF"}
      -DWITH_SYSTEM_MAINTENANCE=#{build.with?("system-maintenance") ? "ON" : "OFF"}
      -DWITH_TEXT_BLOCK=#{text_block ? "ON" : "OFF"}
      -DWITH_TRY_HARD=#{build.with?("try-hard") ? "ON" : "OFF"}
      -DWITH_UNIX_TIMESTAMP_TOOLS=#{unix_timestamp_tools ? "ON" : "OFF"}
      -DWITH_X509_TOOLS=#{x509_tools ? "ON" : "OFF"}
    ]

    system "cmake", "-S", ".", "-B", "build", *args
    system "cmake", "--build", "build"
    system "cmake", "--install", "build"
  end

  test do
    assert_path_exists bin/"get-system-info" if build.with?("get-system-info") || build.with?("system-info")
  end
end
