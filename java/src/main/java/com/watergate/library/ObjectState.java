package com.watergate.library;


/**
 * Created by subho on 16/10/16.
 */
public class ObjectState {
	public static enum EObjectState {
		Unknown, Initialized, Available, Disposed, Exception;

		public static EObjectState parse(int state) {
			switch (state) {
				case 0:
					return Unknown;
				case 1:
					return Initialized;
				case 2:
					return Available;
				case 3:
					return Disposed;
				case 4:
					return Exception;
			}
			return Unknown;
		}
	}

	public static class StateException extends Exception {
		private static final String _PREFIX_ = "Object State Error : ";

		public StateException(String mesg) {
			super(String.format("%s %s", _PREFIX_, mesg));
		}

		public StateException(String mesg, Throwable inner) {
			super(String.format("%s %s", _PREFIX_, mesg), inner);
		}
	}

	private EObjectState state = EObjectState.Unknown;

	private Throwable error = null;

	public ObjectState setState(EObjectState state) {
		this.state = state;

		return this;
	}

	public boolean isAvailable() {
		return (state == EObjectState.Available);
	}

	public boolean isDisposed() {
		return (state == EObjectState.Disposed);
	}

	public boolean hasError() {
		return (state == EObjectState.Exception);
	}

	public ObjectState setError(Throwable error) {
		this.state = EObjectState.Exception;
		this.error = error;

		return this;
	}

	public Throwable getError() {
		if (state == EObjectState.Exception) {
			return error;
		}
		return null;
	}

	public static void CHECK_STATE(ObjectState source, EObjectState
			requested) throws StateException {
		if (source.state != requested) {
			throw new StateException(String.format("Object state doesn't " +
							"match. [source=%s][requested=%s]", source.state.name(),
					requested.name()));
		}
	}
}
