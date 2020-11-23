%define version 1.1.0b
%define release 3

Summary: hengband %{version}
Name: hengband
Version: %{version}
Release: %{release}
Copyright: unknown
Group: Amusements/Games
Packager: Takahiro MIZUNO <tow@plum.freemail.ne.jp>
Url: http://echizen.s5.xrea.com/heng/index.html
Source: hengband-%{version}.tar.bz2
Buildroot: %{_tmppath}/%{name}-%{version}-root

%description
Hengband is a variant of ZAngband.

Official page is this,
http://echizen.s5.xrea.com/heng/eng-hengband/index.html

More infomation is /usr/doc/hengband-hoge/readme_eng.txt

Summary(ja): 変愚蛮怒 %{version}

%description -l ja
変愚蛮怒は Angband のバリアントです。

本ソフトウェアの最新版は以下の場所から入手できます。
http://echizen.s5.xrea.com/heng/index.html

詳しくは /usr/doc/hengband-hoge/readme.txt を参照。

%prep
rm -rf $RPM_BUILD_ROOT

%setup -n %{name}-%{version}

%build
./configure --prefix=%{_prefix} --bindir=%{_bindir} --with-libpath=%{_datadir}/games/hengband/lib
make

%install
mkdir -p $RPM_BUILD_ROOT/%{_bindir}
mkdir -p $RPM_BUILD_ROOT/%{_datadir}/games/hengband
cp src/hengband $RPM_BUILD_ROOT/%{_bindir}
cp -R lib/ -p $RPM_BUILD_ROOT/%{_datadir}/games/hengband/
touch $RPM_BUILD_ROOT/%{_datadir}/games/hengband/lib/apex/scores.raw

%clean
rm -rf $RPM_BUILD_ROOT

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
%{_datadir}/games/hengband/lib/bone/delete.me
%{_datadir}/games/hengband/lib/data/delete.me
%{_datadir}/games/hengband/lib/edit/*.txt
%{_datadir}/games/hengband/lib/file/*.txt
%{_datadir}/games/hengband/lib/help/*.hlp
%{_datadir}/games/hengband/lib/help/*.txt
%{_datadir}/games/hengband/lib/info/delete.me
%{_datadir}/games/hengband/lib/pref/*.prf
%{_datadir}/games/hengband/lib/save/delete.me
%{_datadir}/games/hengband/lib/script/delete.me
%{_datadir}/games/hengband/lib/user/delete.me
%{_datadir}/games/hengband/lib/xtra/graf/8x8.bmp
%doc readme.txt readme_angband readme_eng.txt


%changelog

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

