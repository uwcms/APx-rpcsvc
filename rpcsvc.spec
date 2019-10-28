#define pkg_version provided by makefile
%define name rpcsvc

Name:           %{name}
Version:        %{pkg_version}
Release:        1%{?dist}
Summary:        The APx Gen 1 RPC Service

License:        Reserved
URL:            https://github.com/uwcms/APx-%{name}
Source0:        %{name}-%{pkg_version}.tar.gz

BuildRequires:  protobuf-lite protobuf-lite-devel protobuf-c-compiler libz.so.1 libwisci2c.so.1
BuildRequires:  systemd
Requires:       protobuf-lite libz.so.1

%description
The APx Gen 1 RPC Service provides a remote procedure call interface,
allowing custom modules to be written to easily expose firmware-specific
functionality to higher level subsystem control or monitoring applications.

%package devel
Summary:        The APx Gen 1 RPC Service module development resources

%description devel
The APx Gen 1 RPC Service provides a remote procedure call interface,
allowing custom modules to be written to easily expose firmware-specific
functionality to higher level subsystem control or monitoring applications.

This package provides the headers required to develop service modules for the
APx Gen 1 RPC Service.

%prep
%setup -q


%build
##configure
make %{?_smp_mflags} LIB_VERSION=%{lib_version} RPCSVC_ACL_PATH=%{_sysconfdir}/rpcsvc.ipacl RPCSVC_MODULES_DIR=%{_libdir}/rpcsvc


%install
rm -rf $RPM_BUILD_ROOT
install -D -m 0755 rpcsvc %{buildroot}/%{_bindir}/rpcsvc
install -d %{buildroot}/%{libdir}/rpcsvc
for I in modules/*.so; do
	install -D -m 0755 "$I" "%{buildroot}/%{_libdir}/rpcsvc/${I#modules/}"
done
install -D -m 0644 rpcsvc.ipacl %{buildroot}/%{_sysconfdir}/rpcsvc.ipacl
install -D -m 0644 rpcsvc.service %{buildroot}/%{_unitdir}/rpcsvc.service

# -devel package
for I in *.h; do
	install -D -m 0644 "$I" %{buildroot}/%{_includedir}/rpcsvc/"$I"
done

%files
%{_bindir}/rpcsvc
%config(noreplace) %{_sysconfdir}/rpcsvc.ipacl
%{_unitdir}/rpcsvc.service
%dir %{_libdir}/rpcsvc
%{_libdir}/rpcsvc/*
%doc packages/module_dev.tbz2
%doc packages/client_dev.tbz2
%doc README.md

%files devel
%dir %{_includedir}/rpcsvc
%{_includedir}/rpcsvc/*.h

%pre
getent group rpcsvc > /dev/null || /usr/sbin/groupadd -r rpcsvc || :
getent passwd rpcsvc > /dev/null || /usr/sbin/useradd -r -g rpcsvc -d / -s /sbin/nologin rpcsvc || :


%post
%systemd_post rpcsvc.service


%preun
%systemd_preun rpcsvc.service


%postun
%systemd_postun_with_restart rpcsvc.service


%changelog
* Fri Oct 25 2019 Jesra Tikalsky <jtikalsky@hep.wisc.edu>
- Initial spec file
