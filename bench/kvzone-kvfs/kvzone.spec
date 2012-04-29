Summary: KVZone is a (K)ey-(V)alue store benchmarking utility.
Name: kvzone
Version: 0.9
Release: 1.NECLA
Source: %{name}-%{version}.tar.bz2
Group: 	System/Benchmark
License: BSD
Packager: Sean Timothy Noonan <sean@nec-labs.com>
BuildRoot: %{_builddir}/%{name}-root
BuildRequires: gcc-c++ tokyocabinet-devel db4-devel sqlite-devel boost-devel
%description
This is KVZone, originally developed at NEC Laboratories America.  You can find the latest release at http://www.nec-labs.com/research/robust/robust_grid-website/software.php.  KVZone was created to fill a gap in benchmarking utilities, namely the lack of a standardized, extensible framework for testing and comparing Key-Value stores.  You can read about this process in our HotStorage '10 (http://www.usenix.org/events/hotstorage10/tech/) paper entitled 'KVZone and the Search for a Write-Optimized Key-Value Store.'

%prep
%setup -q

%build
./configure --prefix=$RPM_BUILD_ROOT --with-boost-date-time=boost_date_time --with-boost-filesystem=boost_filesystem --with-boost-thread=boost_thread
make %{?_smp_mflags}

%install
%{makeinstall}

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-, root, root)
%{_bindir}/*
