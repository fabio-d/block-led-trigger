Name:		block-led-trigger
Version:	0.5
Release:	1%{?dist}
Summary:	Block device activity LED trigger
License:	GPLv2

BuildArch:	noarch
Source:		block-led-trigger.tar

BuildRequires:		systemd 
Requires:		kernel-devel
Requires(post):		systemd
Requires(preun):	systemd
Requires(postun):	systemd

%description
Block device activity LED trigger

%prep
%setup -n block-led-trigger

%build

%install
mkdir -p %{buildroot}%{_prefix}/share/block-led-trigger
cp -v block-led-trigger Makefile block_led_trigger.c %{buildroot}%{_prefix}/share/block-led-trigger/

mkdir -p %{buildroot}%{_unitdir}
cp -v block-led-trigger.service %{buildroot}%{_unitdir}

%clean
rm -rf %{buildroot}

%post
%systemd_post block-led-trigger.service

%preun
%systemd_preun block-led-trigger.service

%postun
%systemd_postun_with_restart block-led-trigger.service

%files
%defattr(-,root,root,-)
%{_prefix}/share/block-led-trigger
%{_unitdir}/block-led-trigger.service

%changelog
* Fri Jun 10 2022 Fabio D'Urso <fabiodurso@hotmail.it> 0.5-1
- Make it work with newer kernels.

* Sat Aug 7 2021 Fabio D'Urso <fabiodurso@hotmail.it> 0.4-1
- Make it work with newer kernels.

* Wed Aug 23 2017 Fabio D'Urso <fabiodurso@hotmail.it> 0.3-1
- Fix for Fedora 26.

* Tue Mar 28 2017 Fabio D'Urso <fabiodurso@hotmail.it> 0.2-1
- Search __tracepoint_block_rq_issue in /proc/kallsyms instead of System.map.

* Wed Aug 5 2015 Fabio D'Urso <fabiodurso@hotmail.it> 0.1-1
- Initial version.
