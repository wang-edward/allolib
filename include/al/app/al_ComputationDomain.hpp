#ifndef COMPUTATIONDOMAIN_H
#define COMPUTATIONDOMAIN_H

#include <cassert>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <stack>
#include <vector>

#include "al/app/al_NodeConfiguration.hpp"
#include "al/ui/al_Parameter.hpp"

namespace al {

class ComputationDomain;
class SynchronousDomain;

class DomainMember {
public:
  virtual ComputationDomain *getDefaultDomain() { return nullptr; }
  virtual void registerWithDomain(ComputationDomain *domain = nullptr);
  virtual void unregisterFromDomain(ComputationDomain *domain = nullptr);

protected:
  DomainMember() {}
  ComputationDomain *mParentDomain{nullptr};
};

/**
 * @brief ComputationDomain class
 * @ingroup App
 */
class ComputationDomain {
public:
  virtual ~ComputationDomain() {}
  /**
   * @brief initialize
   * @param parent
   * @return true if init succeeded
   *
   * Multiple calls to init() should be allowed.
   * You should always call this function within child classes to esnure
   * internal state is correct
   */
  virtual bool init(ComputationDomain *parent = nullptr);

  /**
   * @brief cleanup
   * @param parent
   * @return true if cleanup succesfull
   *
   * You should always call this function within child classes to esnure
   * internal state is correct
   */
  virtual bool cleanup(ComputationDomain *parent = nullptr);

  template <class DomainType>
  /**
   * @brief Add a synchronous domain to this domain
   * @param prepend Determines whether the sub-domain should run before or after
   * this domain
   * @return the created domain.
   *
   * The domain specified by DomainType must inherit from SynchronousDomian.
   * It will trigger an std::runtime_error exception if it doesn't.
   * This operation is thread safe if the domain can pause
   * itself, but if it is blocking, so there are no guarantees that it will not
   * interefere with the timely running of the domain.
   *
   * The domain will be initialized if this domain has been initialized.
   *
   * Will return nullptr if this domain is running and the sub domain failed
   * initialization.
   *
   * This function calls addSubDomain() once the domain has been created.
   */
  std::shared_ptr<DomainType> newSubDomain(bool prepend = false);

  /**
   * @brief Inserts subDomain as a subdomain of this class
   * @param subDomain sub domain to insert
   * @param prepend determines if subdomainshould run before or after this
   * domain
   * @return true if sub domain was inserted sucessfully
   *
   * It is the caller's responsibility to ensure the domain is initialized and
   * ready if this domain is already running.
   *
   * This function is thread safe, and will block until the domain has been
   * added. The addtion takes place when the parent unlocks the subdomains, at
   * the end of calls to tickSubdomains().
   * It may block indefinitely if the parent domain does not release the
   * sub domain locks.
   *
   * Subdomain initialization and cleanup will be handled by the parent domain.
   * Initialization will occur on parent initialization if parent is
   * asynchronous, or when inserted if parent is running.
   * If parent is Synchronous, initialization occurs immediately.
   */
  virtual bool addSubDomain(std::shared_ptr<SynchronousDomain> subDomain,
                            bool prepend = false);

  /**
   * @brief Remove a subdomain
   * @param subDomain if nullptr all subdomains are removed
   * @return true if sub domain was inserted sucessfully
   *
   * This operation is thread safe, but it might block causing drops or missed
   * deadlines for the parent domain. If this is a problem, the domain should be
   * stopped prior to adding/removing sub-domains.
   */
  virtual bool
  removeSubDomain(std::shared_ptr<SynchronousDomain> subDomain = nullptr);

  /**
   * @brief Return time delta with respect to previous processing pass for this
   * domain
   * @return time delta
   *
   * The time delta may be set internally by the domain so that it is available
   * whenever the domain ticks. It might be 0 for domains that don't support
   * this functionality or that are tied to strict hardware clocks where this
   * information is not necessary
   */
  double timeDelta() { return mTimeDrift; }

  void setTimeDelta(double delta) { mTimeDrift = delta; }

  Capability getCapabilities() { return mCapabilities; }

  /**
   * @brief register callbacks to be called in the init() function
   * @param callback
   */
  void
  registerInitializeCallback(std::function<void(ComputationDomain *)> callback);

  /**
   * @brief register callbacks to be called in the cleanup() function
   * @param callback
   */
  void
  registerCleanupCallback(std::function<void(ComputationDomain *)> callback);

  /**
   * @brief Return a list of parameters that control this domain
   * @return list of parameters
   *
   * The parameters provided here provide runtime "continuous" parameters,
   * for example like audio gain or eye separation. There should be a clear
   * distinction between values that need to be set on domain intialization
   * that must remain immutable during domain operation and parameters
   * provided here that provide continuous adjustment to the domain's
   * operation.
   */
  std::vector<ParameterMeta *> parameters() { return mParameters; }

  static ComputationDomain *getDomain(std::string tag, size_t index = 0);

  virtual bool registerObject(void *object) { return true; }
  virtual bool unregisterObject(void *object) { return true; }

protected:
  /**
   * @brief initializeSubdomains should be called within the domain's
   * initialization function
   * @param pre initialize prepended domains if true, otherwise appended
   * domains.
   * @return true if all initializations sucessful.
   *
   * You must call this function twice: once for prepended and then for
   * appended domains.
   */
  bool initializeSubdomains(bool pre = false);

  /**
   * @brief execute subdomains
   * @param pre execute prepended domains if true, otherwise appended domains.
   * @return true if all execution sucessful.
   *
   * You must call this function twice: once for prepended and then for
   * appended domains.
   */
  bool tickSubdomains(bool pre = false);

  /**
   * @brief cleanup subdomains
   * @param pre cleanup prepended domains if true, otherwise appended domains.
   * @return true if all cleanup sucessful.
   *
   * You must call this function twice: once for prepended and then for
   * appended domains.
   */
  bool cleanupSubdomains(bool pre = false);

  /**
   * @brief callInitializeCallbacks should be called by children of this class
   * after the domain has been initialized
   */
  void callInitializeCallbacks();

  /**
   * @brief callInitializeCallbacks should be called by children of this class
   * before the domain has been cleaned up
   */
  void callCleanupCallbacks();

  double mTimeDrift{0.0};
  std::vector<std::pair<std::shared_ptr<SynchronousDomain>, bool>>
      mSubDomainList;

  // Add parameters for domain control here
  std::vector<ParameterMeta *> mParameters;
  bool mInitialized{false};

  // Global singleton domain manager
  static std::vector<std::pair<ComputationDomain *, std::string>>
      mPublicDomains;
  static std::mutex mPublicDomainsLock;
  void addPublicDomain(ComputationDomain *domain, std::string tag);

private:
  std::vector<std::function<void(ComputationDomain *)>> mInitializeCallbacks;
  std::vector<std::function<void(ComputationDomain *)>> mCleanupCallbacks;

  std::mutex mSubdomainLock;

  Capability mCapabilities;
};

class SynchronousDomain : public ComputationDomain {
  friend class ComputationDomain;

public:
  /**
   * @brief Execute a pass of the domain.
   * @return true if execution of the domain succeeded
   */
  virtual bool tick();
};

/**
 * @brief The AsynchronousDomain class manages a domain that is run
 * asynchronously, but the threading is managed elasewhere
 *
 * If you need a domain where the thread and concurrency are managed use
 * AysnchronousThreadDomain.
 */
class AsynchronousDomain : public ComputationDomain {
public:
  /**
   * @brief start the asyncrhonous execution of the domain
   * @return true if start was successful
   *
   * Assumes that init() has already been called. When start exits, the domain
   * is considered stopped, but still initialized. Domains must ensure that this
   * is done internally. In cases where this domain blocks, the stop function
   * can only be called from a separate thread and must be thread safe.
   *
   */
  virtual bool start() = 0;

  /**
   * @brief stop the asyncrhonous execution of the domain
   * @return true if stop was successful
   *
   * Domains should be written so that both start() or cleanup()
   * work after calling stop()
   */
  virtual bool stop() = 0;

protected:
  /**
   * @brief callStartCallbacks should be called by children of this class
   * after the domain has been set up to start, before going into the blocking
   * loop
   */
  void callStartCallbacks();

  /**
   * @brief callStopCallbacks should be called by children of this class on
   * the stop request, before the domain has been stopped
   */
  void callStopCallbacks() {
    for (auto callback : mStopCallbacks) {
      callback(this);
    }
  }

private:
  std::vector<std::function<void(ComputationDomain *)>> mStartCallbacks;
  std::vector<std::function<void(ComputationDomain *)>> mStopCallbacks;
};

class AsynchronousThreadDomain : public AsynchronousDomain {
public:
  /**
   * @brief start the domain on a separate thread so it won't block
   * @return true if start was successful or if already running async
   *
   * Assumes that init() has already been called. This call will not block. You
   * can wait on the domain through the std::future provided by waitForDomain()
   */
  virtual bool start() = 0;

  std::future<bool> &waitForDomain();

  /**
   * @brief stop the threaded execution of the domain
   * @return true if stop was successful
   *
   * Only call this function if domain was started with startAsync()
   */
  virtual bool stop() = 0;

protected:
  std::promise<bool> mDomainAsyncInitPromise;
  std::future<bool> mDomainAsyncInit;

  std::promise<bool> mDomainAsyncResultPromise;
  std::future<bool> mDomainAsyncResult;

private:
  std::unique_ptr<std::thread> mAsyncThread;
};

template <class DomainType>
std::shared_ptr<DomainType> ComputationDomain::newSubDomain(bool prepend) {
  auto newDomain = std::make_shared<DomainType>();
  if (!dynamic_cast<SynchronousDomain *>(newDomain.get())) {
    // Only Synchronous domains are allowed as subdomains
    throw std::runtime_error(
        "Subdomain must be a subclass of SynchronousDomain");
  }
  if (mInitialized) {
    if (newDomain->init(this)) {
      addSubDomain(newDomain, prepend);
    } else {
      newDomain = nullptr;
    }
  } else {
    addSubDomain(newDomain, prepend);
  }
  return newDomain;
}

} // namespace al
#endif // COMPUTATIONDOMAIN_H
