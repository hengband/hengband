%define version 1.1.0b
%define release 3

Summary: Bakabakaband %{version}
Name: Bakabakaband
Version: %{version}
Release: %{release}
Copyright: unknown
Group: Amusements/Games
Packager: Takahiro MIZUNO <tow@plum.freemail.ne.jp>
Url: http://echizen.s5.xrea.com/heng/index.html
Source: Bakabakaband-%{version}.tar.bz2
Buildroot: %{_tmppath}/%{name}-%{version}-root

%description
Bakabakaband is a variant of ZAngband.

Official page is this,
http://echizen.s5.xrea.com/heng/eng-Bakabakaband/index.html

More infomation is /usr/doc/Bakabakaband-hoge/readme_eng.txt

Summary(ja): �Ѷ����� %{version}

%description -l ja
�Ѷ����ܤ� Angband �ΥХꥢ��ȤǤ���

�ܥ��եȥ������κǿ��Ǥϰʲ��ξ�꤫������Ǥ��ޤ���
http://echizen.s5.xrea.com/heng/index.html

�ܤ�����Bakabakabandc/hengband-hoge/readme.txt �򻲾ȡ�

%prep
rm -rf $RPM_BUILD_ROOT

%setup -n %{name}-%{version}

%build
./configure --prefix=%{_prefix} --bindir=%{_bindir} --with-libpath=%{_datadir}/games/Bakabakaband/lib
make

%install
mkdir -p $RPM_BUILD_ROOT/%{_bindir}
mkdir -p $RPM_BUILD_ROOT/%{_datadir}/games/Bakabakaband
cp src/Bakabakaband $RPM_BUILD_ROOT/%{_bindir}
cp -R lib/ -p $RPM_BUILD_ROOT/%{_datadir}/games/Bakabakaband/
touch $RPM_BUILD_ROOT/%{_datadir}/games/Bakabakaband/lib/apex/scores.raw

%clean
rm -rf $RPM_BUILD_ROOT

%preun
if [ -e %{_datadir}/games/Bakabakaband/lib/data/f_info_j.raw ]
then
rm -rf %{_datadir}/games/Bakabakaband/lib/data/*.raw
fi
exit 0

%files
%defattr(-,root,root)
%attr(2755,root,games) %{_bindir}/Bakabakaband
%dir %{_datadir}/games/Bakabakaband/lib
%attr(775,root,games) %dir %{_datadir}/games/Bakabakaband/lib/apex
%attr(775,root,games) %dir %{_datadir}/games/Bakabakaband/lib/bone
%attr(775,root,games) %dir %{_datadir}/games/Bakabakaband/lib/data
%dir %{_datadir}/games/Bakabakaband/lib/edit
%dir %{_datadir}/games/Bakabakaband/lib/file
%dir %{_datadir}/games/Bakabakaband/lib/help
%dir %{_datadir}/games/Bakabakaband/lib/info
%dir %{_datadir}/games/Bakabakaband/lib/pref
%attr(775,root,games) %dir %{_datadir}/games/Bakabakaband/lib/save
%dir %{_datadir}/games/Bakabakaband/lib/script
%dir %{_datadir}/games/Bakabakaband/lib/user
%dir %{_datadir}/games/Bakabakaband/lib/xtra
%dir %{_datadir}/games/Bakabakaband/lib/xtra/graf
%{_datadir}/games/Bakabakaband/lib/apex/h_scores.raw
%{_datadir}/games/Bakabakaband/lib/apex/readme.txt
%attr(664 root,games) %config(noreplace) %{_datadir}/games/Bakabakaband/lib/apex/scores.raw
%{_datadir}/games/Bakabakaband/lib/bone/delete.me
%{_datadir}/games/Bakabakaband/lib/data/delete.me
%{_datadir}/games/Bakabakaband/lib/edit/*.txt
%{_datadir}/games/Bakabakaband/lib/file/*.txt
%{_datadir}/games/Bakabakaband/lib/help/*.hlp
%{_datadir}/games/Bakabakaband/lib/help/*.txt
%{_datadir}/games/Bakabakaband/lib/info/delete.me
%{_datadir}/games/Bakabakaband/lib/pref/*.prf
%{_datadir}/games/Bakabakaband/lib/save/delete.me
%{_datadir}/games/Bakabakaband/lib/script/delete.me
%{_datadir}/games/Bakabakaband/lib/user/delete.me
%{_datadir}/games/Bakabakaband/lib/xtra/graf/8x8.bmp
%doc readme.txt readme_angband readme_eng.txt


%changelog

* Fri Jul 05 2002 Takahiro MIZUNO <tow@plum.freemail.ne.jp>
- Bakabakaband RPM 1.0.0b release 3
- Add %preun script.
- Change source extension. (tar.gz -> bz2)
- Fix Copyright.
- Fix simply %files.
- Fix %description.

* Mon Jun 17 2002 Takahiro MIZUNO <tow@plum.freemail.ne.jp>
- Bakabakaband RPM 1.0.0b release 2
- Fix setgid permission. (Mogami����¿��)

* Sun Jun 16 2002 Takahiro MIZUNO <tow@plum.freemail.ne.jp>
- Bakabakaband RPM 1.0.0b release 1

* Sun Jun 16 2002 Takahiro MIZUNO <tow@plum.freemail.ne.jp> 
- Bakabakaband RPM 1.0.0 release 1

