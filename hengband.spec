%define version 3.0.1.0
%define release 1

Summary: hengband %{version}
Name: hengband
Version: %{version}
Release: %{release}
License: unknown
Group: Amusements/Games
Url: https://hengband.github.io
Source: hengband-%{version}.tar.gz
Requires: ncurses-libs libstdc++ libcurl libX11
BuildRequires: autoconf automake gcc-c++ ncurses-devel libcurl-devel nkf libX11-devel

%description
Hengband is a variant of ZAngband.

Official page is this,
https://hengband.github.io

More infomation is /usr/share/doc/hengband/readme-eng.md

Summary(ja): 変愚蛮怒 %{version}

%description -l ja
変愚蛮怒は Angband のバリアントです。

本ソフトウェアの最新版は以下の場所から入手できます。
https://hengband.github.io

詳しくは /usr/share/doc/hengband/readme.md を参照。

%prep
rm -rf %{buildroot}

%setup -n %{name}-%{version}
./bootstrap

%build
%configure --with-libpath=%{_datadir}/games/hengband/lib
%make_build

%install
mkdir -p %{buildroot}/%{_bindir}
mkdir -p %{buildroot}/%{_datadir}/games/hengband
%make_install bindir=%{_bindir}
cp -R lib/ -p %{buildroot}/%{_datadir}/games/hengband/
find %{buildroot}/%{_datadir}/games/hengband/ -type f -name "Makefile*" -exec rm {} \;
find %{buildroot}/%{_datadir}/games/hengband/ -type f -name "delete.me*" -exec rm {} \;
find %{buildroot}/%{_datadir}/games/hengband/ -name ".git*" -exec rm -rf {} \;
rm -rf %{buildroot}/%{_datadir}/games/hengband/lib/xtra/{sound,music}
touch %{buildroot}/%{_datadir}/games/hengband/lib/apex/scores.raw

%clean
rm -rf %{buildroot}

%preun
if [ -e %{_datadir}/games/hengband/lib/data/f_info_j.raw ]
then
rm -rf %{_datadir}/games/hengband/lib/data/*.raw
fi
exit 0

%files
%defattr(-,root,root)
%attr(2755,root,games) %{_bindir}/hengband
%dir %{_datadir}/games/hengband/lib
%attr(775,root,games) %dir %{_datadir}/games/hengband/lib/apex
%attr(775,root,games) %dir %{_datadir}/games/hengband/lib/bone
%attr(775,root,games) %dir %{_datadir}/games/hengband/lib/data
%dir %{_datadir}/games/hengband/lib/edit
%dir %{_datadir}/games/hengband/lib/file
%dir %{_datadir}/games/hengband/lib/file/books
%dir %{_datadir}/games/hengband/lib/help
%dir %{_datadir}/games/hengband/lib/info
%dir %{_datadir}/games/hengband/lib/pref
%attr(775,root,games) %dir %{_datadir}/games/hengband/lib/save
%dir %{_datadir}/games/hengband/lib/script
%dir %{_datadir}/games/hengband/lib/user
%dir %{_datadir}/games/hengband/lib/xtra
%dir %{_datadir}/games/hengband/lib/xtra/graf
%{_datadir}/games/hengband/lib/apex/h_scores.raw
%{_datadir}/games/hengband/lib/apex/readme.txt
%attr(664 root,games) %config(noreplace) %{_datadir}/games/hengband/lib/apex/scores.raw
%{_datadir}/games/hengband/lib/edit/*.txt
%{_datadir}/games/hengband/lib/edit/quests/*.txt
%{_datadir}/games/hengband/lib/edit/towns/*.txt
%{_datadir}/games/hengband/lib/file/*.txt
%{_datadir}/games/hengband/lib/file/books/*.txt
%{_datadir}/games/hengband/lib/help/*.hlp
%{_datadir}/games/hengband/lib/help/*.txt
%{_datadir}/games/hengband/lib/pref/*.prf
%{_datadir}/games/hengband/lib/xtra/graf/8x8.bmp
%doc readme.md readme_angband readme-eng.md
%license lib/help/jlicense.txt

%changelog

* Mon Oct 30 2023 Shiro Hara <white@vx-xv.com>
- hengband RPM 3.0.1.0(Beta)

* Sun Oct 22 2023 Shiro Hara <white@vx-xv.com>
- Fix the graphic mode is not available on X11

* Wed Oct 18 2023 Shiro Hara <white@vx-xv.com>
- hengband RPM 3.0.0.91(Alpha)

* Mon Oct 16 2023 Shiro Hara <white@vx-xv.com>
- hengband RPM 3.0.0.90(Alpha)

* Tue Aug 8 2023 Shiro Hara <white@vx-xv.com>
- hengband RPM 3.0.0.89(Alpha)

* Mon Jul 24 2023 Shiro Hara <white@vx-xv.com>
- Enable X11

* Sun Jul 23 2023 Shiro Hara <white@vx-xv.com>
- hengband RPM 3.0.0.88(Alpha)

* Sun Jul 09 2023 Shiro Hara <white@vx-xv.com>
- hengband RPM 3.0.0.87(Alpha)

* Mon Jun 26 2023 Shiro Hara <white@vx-xv.com>
- hengband RPM 3.0.0.86(Alpha)

* Wed Jun 14 2023 Shiro Hara <white@vx-xv.com>
- hengband RPM 3.0.0.85(Alpha)

* Mon May 29 2023 Shiro Hara <white@vx-xv.com>
- hengband RPM 3.0.0Alpha release 84

* Wed May 17 2023 Shiro Hara <white@vx-xv.com>
- hengband RPM 3.0.0Alpha release 83
- Replace RPM_BUILD_ROOT to builddir macro

* Sat May 06 2023 Shiro Hara <white@vx-xv.com>
- hengband RPM 3.0.0Alpha release 82

* Thu May 04 2023 Shiro Hara <white@vx-xv.com>
- hengband RPM 3.0.0Alpha release 81

* Mon Feb 20 2023 Shiro Hara <white@vx-xv.com>
- hengband RPM 3.0.0Alpha release 78

* Sun Feb 19 2023 Shiro Hara <white@vx-xv.com>
- hengband RPM 3.0.0Alpha release 77
- Remove Packacger
- Remove Buildroot
- Add %license
- Fix Version and Release

* Fri Feb 17 2023 Shiro Hara <white@vx-xv.com>
- hengband RPM 3.0.0Alpha release 76
- Renew Url
- Renew Packager
- Change Copyright to License
- Change readme.txt to readme.md

* Fri Jul 05 2002 Takahiro MIZUNO <tow@plum.freemail.ne.jp>
- hengband RPM 1.0.0b release 3
- Add %preun script.
- Change source extension. (tar.gz -> bz2)
- Fix Copyright.
- Fix simply %files.
- Fix %description.

* Mon Jun 17 2002 Takahiro MIZUNO <tow@plum.freemail.ne.jp>
- hengband RPM 1.0.0b release 2
- Fix setgid permission. (Mogamiさん多謝)

* Sun Jun 16 2002 Takahiro MIZUNO <tow@plum.freemail.ne.jp>
- hengband RPM 1.0.0b release 1

* Sun Jun 16 2002 Takahiro MIZUNO <tow@plum.freemail.ne.jp> 
- hengband RPM 1.0.0 release 1
