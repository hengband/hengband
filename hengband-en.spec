%define version 3.0.1.8
%define release 1

Summary: hengband %{version}
Name: hengband-en
Version: %{version}
Release: %{release}
License: unknown
Group: Amusements/Games
Url: https://hengband.github.io
Source: hengband-%{version}.tar.gz
Requires: ncurses-libs libstdc++ libcurl libX11
BuildRequires: autoconf automake gcc-c++ ncurses-devel libcurl-devel libX11-devel

%description
Hengband is a variant of ZAngband.

Official page is this,
https://hengband.github.io

More infomation is /usr/share/doc/hengband-en/readme-eng.md
This is an English version package.

Summary(ja): 変愚蛮怒 %{version}

%description -l ja
変愚蛮怒は Angband のバリアントです。

本ソフトウェアの最新版は以下の場所から入手できます。
https://hengband.github.io

詳しくは /usr/share/doc/hengband-en/readme.md を参照。
このパッケージは英語版です。

%prep
rm -rf %{buildroot}

%setup -n hengband-%{version}
./bootstrap

%build
%configure --with-libpath=%{_datadir}/games/%{name}/lib --disable-japanese
%make_build

%install
mkdir -p %{buildroot}/%{_bindir}
mkdir -p %{buildroot}/%{_datadir}/games/%{name}
%make_install bindir=%{_bindir}
mv %{buildroot}/%{_bindir}/hengband %{buildroot}/%{_bindir}/hengband-en
cp -R lib/ -p %{buildroot}/%{_datadir}/games/%{name}/
find %{buildroot}/%{_datadir}/games/%{name}/ -type f -name "Makefile*" -exec rm {} \;
find %{buildroot}/%{_datadir}/games/%{name}/ -type f -name "delete.me*" -exec rm {} \;
find %{buildroot}/%{_datadir}/games/%{name}/ -name ".git*" -exec rm -rf {} \;
rm -rf %{buildroot}/%{_datadir}/games/%{name}/lib/xtra/{sound,music}
touch %{buildroot}/%{_datadir}/games/%{name}/lib/apex/scores.raw

%clean
rm -rf %{buildroot}

%preun
if [ -e %{_datadir}/games/%{name}/lib/data/f_info_j.raw ]
then
rm -rf %{_datadir}/games/%{name}/lib/data/*.raw
fi
exit 0

%files
%defattr(-,root,root)
%attr(2755,root,games) %{_bindir}/%{name}
%dir %{_datadir}/games/%{name}/lib
%attr(775,root,games) %dir %{_datadir}/games/%{name}/lib/apex
%attr(775,root,games) %dir %{_datadir}/games/%{name}/lib/bone
%attr(775,root,games) %dir %{_datadir}/games/%{name}/lib/data
%dir %{_datadir}/games/%{name}/lib/edit
%dir %{_datadir}/games/%{name}/lib/file
%dir %{_datadir}/games/%{name}/lib/file/books
%dir %{_datadir}/games/%{name}/lib/help
%dir %{_datadir}/games/%{name}/lib/info
%dir %{_datadir}/games/%{name}/lib/pref
%attr(775,root,games) %dir %{_datadir}/games/%{name}/lib/save
%dir %{_datadir}/games/%{name}/lib/script
%dir %{_datadir}/games/%{name}/lib/user
%dir %{_datadir}/games/%{name}/lib/xtra
%dir %{_datadir}/games/%{name}/lib/xtra/graf
%{_datadir}/games/%{name}/lib/apex/h_scores.raw
%{_datadir}/games/%{name}/lib/apex/readme.txt
%attr(664 root,games) %config(noreplace) %{_datadir}/games/%{name}/lib/apex/scores.raw
%{_datadir}/games/%{name}/lib/edit/*.txt
%{_datadir}/games/%{name}/lib/edit/quests/*.txt
%{_datadir}/games/%{name}/lib/edit/towns/*.txt
%{_datadir}/games/%{name}/lib/file/*.txt
%{_datadir}/games/%{name}/lib/file/books/*.txt
%{_datadir}/games/%{name}/lib/help/*.hlp
%{_datadir}/games/%{name}/lib/help/*.txt
%{_datadir}/games/%{name}/lib/pref/*.prf
%{_datadir}/games/%{name}/lib/xtra/graf/8x8.bmp
%doc readme.md readme_angband readme-eng.md
%license lib/help/jlicense.txt

%changelog
* Thu Mar 07 2024 Shiro Hara <white@vx-xv.com>
- English version
- hengband-en RPM 3.0.1.8(Beta)
