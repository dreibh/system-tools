class TdSystemTools < Formula
  desc "Utilities for system management and maintenance"
  homepage "https://www.nntb.no/~dreibh/system-tools/"
  url "https://www.nntb.no/~dreibh/system-tools/download/td-system-tools-2.5.4.tar.xz"
  sha256 "REPLACE_WITH_ACTUAL_SHA256_OF_THE_TAR_XZ"
  license "GPL-3.0-or-later"

  # Build dependencies
  depends_on "cmake" => :build

  # Runtime dependencies
  depends_on "bash"
  depends_on "figlet"
  depends_on "gettext"
  depends_on "mbuffer"
  depends_on "openssl@3"

  def install
    args = std_cmake_args + %w[
      -DCMAKE_INSTALL_SYSCONFDIR=#{etc}
      -DWITH_SYSTEM_MAINTENANCE=OFF
      -DWITH_CONFIGURE_GRUB=OFF
      -DWITH_RESET_MACHINE_ID=OFF
    ]

    system "cmake", "-S", ".", "-B", "build", *args
    system "cmake", "--build", "build"
    system "cmake", "--install", "build"
  end

  def caveats
    <<~EOS
      System-Tools configuration files have been installed to:
        #{etc}/system-info.d/
      
      Note: System-Maintenance, Configure-GRUB, and Reset-Machine-ID 
      are disabled on macOS as they are Linux/FreeBSD specific.
    EOS
  end

  test do
    system "#{bin}/print-utf8", "-h"
    system "#{bin}/random-sleep", "0", "0"
  end
end
