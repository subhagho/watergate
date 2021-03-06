package com.watergate.library.io;

import com.watergate.library.LockClientEnv;
import com.watergate.library.LockControlClient;
import com.watergate.library.LockControlClient.ELockResult;
import com.watergate.library.LockControlClient.EResourceType;
import com.watergate.library.LockControlException;
import com.watergate.library.ObjectState.StateException;
import com.watergate.library.utils.LogUtils;

import java.io.*;
import java.util.concurrent.TimeoutException;

/**
 * Created by subho on 17/10/16.
 */
public class PriorityFileOutputStream extends FileOutputStream {
	private LockControlClient lockClient;
	private short priority;
	private String lockname = null;
	private int currentLockCount = 0;
	private long timeout = -1;

	public PriorityFileOutputStream(String name, short priority) throws
			LockControlException, StateException, IOException {
		super(name);
		File f = new File(name);
		setup(priority, f.getCanonicalPath());
	}

	public PriorityFileOutputStream(String name, boolean append, short
			priority)
			throws
			LockControlException, StateException, IOException {
		super(name, append);
		File f = new File(name);
		setup(priority, f.getCanonicalPath());
	}

	public PriorityFileOutputStream(File file, short priority) throws
			LockControlException, StateException, IOException {
		super(file);
		setup(priority, file.getCanonicalPath());
	}

	public PriorityFileOutputStream(File file, boolean append, short priority)
			throws LockControlException,
			StateException, IOException {
		super(file, append);
		setup(priority, file.getCanonicalPath());
	}

	public PriorityFileOutputStream withTimeout(long timeout) {
		this.timeout = timeout;

		return this;
	}

	public short getPriority() {
		return priority;
	}

	private void setup(short priority, String filename) throws
			LockControlException, StateException {
		this.priority = priority;
		this.lockClient = LockClientEnv.getLockClient();

		lockname = lockClient.findLockByName(filename, EResourceType.getValue
				(EResourceType.FS));
		lockClient.register_thread(lockname);
	}

	@Override
	public void write(int b) throws IOException {
		throw new IOException("Method not supported.");
	}

	@Override
	public void write(byte[] b) throws IOException {
		write(b, timeout);
	}

	public void write(byte[] b, long timeout) throws IOException {
		if (lockname != null && !lockname.isEmpty()) {
			try {
				currentLockCount = 0;
				try {
					double quota = lockClient.getQuota(lockname);
					if (b.length <= quota) {
						writeBlock(b, 0, b.length, timeout);
					} else {
						double rem = 0;
						double written = 0;
						while (written < b.length) {
							rem = b.length - written;
							if (rem > 0) {
								if (rem > quota) {
									rem = quota;
								}
								writeBlock(b, (int) written, (int) rem,
										timeout);
								written += rem;
							} else {
								break;
							}
						}
					}
				} finally {
					for (int ii = 0; ii < currentLockCount; ii++) {
						lockClient.release(lockname, priority);
					}
				}
			} catch (Throwable t) {
				throw new IOException(t);
			}
		} else {
			super.write(b);
		}
	}

	@Override
	public void write(byte[] b, int off, int len) throws
			IOException {
		write(b, off, len, timeout);
	}

	public void write(byte[] b, int off, int len, long timeout) throws
			IOException {
		if (lockname != null && !lockname.isEmpty()) {
			try {
				currentLockCount = 0;
				try {
					double quota = lockClient.getQuota(lockname);
					if (len <= quota) {
						writeBlock(b, off, len, timeout);
					} else {
						double rem = 0;
						double written = 0;
						while (written < len) {
							rem = len - written;
							if (rem > 0) {
								if (rem > quota) {
									rem = quota;
								}
								writeBlock(b, (off + (int) written), (int)
										rem, timeout);
								written += rem;
							} else {
								break;
							}
						}
					}
				} finally {
					for (int ii = 0; ii < currentLockCount; ii++) {
						lockClient.release(lockname, priority);
					}
				}
			} catch (Throwable t) {
				throw new IOException(t);
			}
		} else {
			super.write(b, off, len);
		}
	}

	private void writeBlock(byte[] b, int off, int len, long timeout) throws
			IOException,
			LockControlException, TimeoutException {
		ELockResult r = ELockResult.None;
		if (timeout > 0) {
			r = lockClient.getLock(lockname, priority, len, timeout);
		} else
			r = lockClient.getLock(lockname, priority, len);
		LogUtils.debug(getClass(), "Lock result returned [" + r.name
				() + "]");
		if (r == ELockResult.Locked) {
			currentLockCount++;
			super.write(b, off, len);
		} else if (r == ELockResult.Timeout) {
			throw new TimeoutException(String.format("Error writing to " +
					"locked file. [result=%s]", r.name()));
		} else {
			throw new LockControlException(String.format("Error writing to " +
					"locked file. [result=%s]", r.name()));
		}
	}
}
